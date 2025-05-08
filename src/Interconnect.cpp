#include "../include/Interconnect.h"
#include "../include/Memory.h"
#include <iostream>
#include <chrono>
#include <numeric>


// Instancia el recollector de estadísticas
InterconnectStats icstats;

// Constructor e inicialización del Interconnect
Interconnect::Interconnect() : running(false) {}

// Destructor del Interconnect
Interconnect::~Interconnect() {
    stop();
}

// Método para recibir mensajes de las PEs
bool Interconnect::receiveMessage(const SMS& msg) {

    std::lock_guard<std::mutex> lock(queue_mutex);
    if (msg.type == MessageType::INV_ACK) {
        invalidation_queue.push(msg);
        std::cout << "[INTERCONNECT] INV_ACK recibido, encolado en invalidation_queue.\n";
    } else {
        message_queue.push_back(msg);

        if (msg.type == MessageType::WRITE_RESP || msg.type == MessageType::READ_RESP){
            std::cout << "[INTERCONNECT] Mensaje recibido de [MEMORY] para PE" << msg.dest << " esperando a ser procesado...\n";
        }else{
            std::cout << "[INTERCONNECT] Mensaje recibido de PE" << msg.src << " esperando a ser procesado...\n";
        }
        
    }
    cv.notify_one();
    return true;
}

// Metodo para comenzar el hilo de procesamiento
void Interconnect::start() {
    running = true;
    processing_thread = std::thread(&Interconnect::processQueue, this);
}

// Método para detener el hilo de procesamiento
void Interconnect::stop() {
    running = false;
    cv.notify_all();
    if (processing_thread.joinable()) {
        processing_thread.join();
    }
}

// Método para establecer la memoria
void Interconnect::setMemory(Memory* mem) {
    memory = mem;
}

// Método para registrar cada PE
void Interconnect::registerPE(int id, PE* pe) {
    pe_registry[id] = pe;
}

// Método para establecer el modo de calendarización
void Interconnect::setSchedulingMode(bool fifo) {
    fifo_mode = fifo;
}

// Método para establecer el modo de stepping
void Interconnect::setSteppingMode(bool enabled) {
    stepping_mode = enabled;
    step_ready = !enabled;  // Si no está en modo stepping, permitir paso automático
}

// Método para obtener el modo de stepping
bool Interconnect::getSteppingMode() {
    return stepping_mode;
}

// Método para activar el siguiente paso en modo stepping
void Interconnect::triggerNextStep() {
    {
        std::lock_guard<std::mutex> lock(step_mutex);
        step_ready = true;
    }
    step_cv.notify_one();
}

// Método para obtener el estado de ejecución del Interconnect
bool Interconnect::isRunning() const {
    return running;
}

// Método para simular el tiempo de espera sin consumir CPU Time
void Interconnect::wait_until(std::chrono::steady_clock::time_point ready_time) {
    while (std::chrono::steady_clock::now() < ready_time) {
        std::this_thread::yield();
    }
}

// Método para establecer el tiempo de penalización base y el tiempo de penalización por byte
void Interconnect::setPenaltyTimers(double new_penalty_timer, double new_penalty_bytes) {
    PENALTY_TIMER = new_penalty_timer;
    PENALTY_BYTES = new_penalty_bytes;
}

// Método para procesar los mensajes en la cola principal
// Este método se ejecuta en un hilo separado y procesa los mensajes de la cola
// según el modo de calendarización (FIFO o QoS).
void Interconnect::processQueue() {

    auto start_time = std::chrono::steady_clock::now();
    
    while (running || !message_queue.empty() || (memory && !memory->isIdle())) {

        // Esperar hasta que haya mensajes en la cola o el Interconnect esté detenido
        std::unique_lock<std::mutex> lock(queue_mutex);
        cv.wait(lock, [&]() {
            return !message_queue.empty() || !running;
        });
;
        // Si el Interconnect no está en ejecución y la cola está vacía, salir
        if (message_queue.empty()) {
            lock.unlock();  // liberar el mutex antes de chequear condiciones globales

            auto ready_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
            wait_until(ready_time);

            continue;
        }
        

        // Si hay mensajes en la cola, procesar el siguiente mensaje
        // Seleccionar el mensaje a procesar según el modo de calendarización
        SMS msg;
        if (fifo_mode) {
            msg = message_queue.front();
            message_queue.pop_front();
        } else {
            auto best_it = message_queue.begin();
            for (auto it = message_queue.begin(); it != message_queue.end(); ++it) {
                if (it->qos > best_it->qos) {
                    best_it = it;
                }
            }
            msg = *best_it;
            message_queue.erase(best_it);
        }


        // Si el Interconnect está en modo stepping, esperar a que el usuario presione ENTER
        if (stepping_mode) {
            std::cout << "[STEPPING] Esperando ENTER para continuar con la siguiente instrucción...\n";
            std::unique_lock<std::mutex> lock(step_mutex);
            step_cv.wait(lock, [&] { return step_ready; });
            step_ready = false;
        }

        lock.unlock();

        // Antes de procesar cada mensaje se registra el tiempo de inicio
        auto msg_start = std::chrono::steady_clock::now();
        icstats.message_counts[msg.type]++;
        icstats.messages_per_pe[msg.src]++;
        icstats.total_bytes_transferred += msg.calculateSize();


        // Si es BROADCAST_INVALIDATE, manejar toda la lógica especial
        if (msg.type == MessageType::BROADCAST_INVALIDATE) {
            std::cout << "[INTERCONNECT] Procesando BROADCAST_INVALIDATE de PE" << msg.src << " con delay de 0,1s \n";

            // Simular el tiempo de espera para una instrucción de tipo BROADCAST_INVALIDATE
            wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(PENALTY_BYTES));

            // Generar el estado de invalidación
            current_invalidation = InvalidationState{
                .origin_id = msg.src,
                .expected_acks = static_cast<int>(pe_registry.size()) - 1,
                .received_acks = 0,
                .waiting_type = msg.type,
                .original_msg = msg
            };

            // Enviar réplicas a todos los demás PE
            for (const auto& [id, pe] : pe_registry) {
                if (id != msg.src) {
                    SMS replica = msg;
                    replica.dest = id;
                    pe->receiveResponse(replica);
                }
            }

            std::cout << "[INTERCONNECT] Réplicas enviadas. Esperando INV_ACKs...\n";

            // Esperar y procesar los INV_ACK con delay de 0.1s cada uno
            while (current_invalidation->received_acks < current_invalidation->expected_acks) {
                while (invalidation_queue.empty()) {
                    std::this_thread::yield();
                }

                // Procesar el primer INV_ACK en la cola
                SMS ack_msg = invalidation_queue.front();
                invalidation_queue.pop();

                icstats.message_counts[ack_msg.type]++;
                icstats.messages_per_pe[ack_msg.src]++;
                icstats.total_bytes_transferred += ack_msg.calculateSize();

                std::cout << "[INTERCONNECT] Procesando INV_ACK de PE" << ack_msg.src << " (delay 0.1s)\n";
                wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(PENALTY_BYTES));

                current_invalidation->received_acks++;
            }

            // Enviar INV_COMPLETE al PE que originó el broadcast
            std::cout << "[INTERCONNECT] Todos los INV_ACK recibidos. Enviando INV_COMPLETE al PE" << current_invalidation->origin_id << "\n";

            SMS complete_msg;

            complete_msg.type = MessageType::INV_COMPLETE;
            complete_msg.dest = current_invalidation->origin_id;
            complete_msg.qos = current_invalidation->original_msg.qos;

            icstats.message_counts[complete_msg.type]++;
            icstats.messages_per_pe[complete_msg.dest]++;
            icstats.total_bytes_transferred += complete_msg.calculateSize();

            wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(PENALTY_TIMER));


            if (auto it = pe_registry.find(complete_msg.dest); it != pe_registry.end()) {
                it->second->receiveResponse(complete_msg);
            }

            current_invalidation.reset();
            continue;  // Pasar al siguiente ciclo sin procesar más
        }

        // Calcular el tiempo de penalización para los otros tipos de mensajes
        double delay_secs = PENALTY_TIMER/1000;
        if (msg.type == MessageType::WRITE_MEM) {
            delay_secs += msg.num_of_cache_lines * CACHE_WIDTH * (PENALTY_BYTES/1000);
        } else if (msg.type == MessageType::READ_RESP) {
            delay_secs += msg.data.size() * (PENALTY_BYTES/1000);
        } else if (msg.type == MessageType::READ_MEM || msg.type == MessageType::WRITE_RESP) {
            delay_secs += msg.size * (PENALTY_BYTES/1000);
        }

        if (msg.type == MessageType::WRITE_RESP || msg.type == MessageType::READ_RESP){
            std::cout << "[INTERCONNECT] Procesando mensaje de [MEMORY] para  PE" << msg.dest
                  << " con delay de " << delay_secs << "s \n";
        }else{
            std::cout << "[INTERCONNECT] Procesando mensaje de PE" << msg.src
                  << " con delay de " << delay_secs << "s \n";
        }

        
        // Simular el tiempo de espera para la instrucción
        auto ready_time = std::chrono::steady_clock::now() + std::chrono::duration<double>(delay_secs);
        while (std::chrono::steady_clock::now() < ready_time) {
            std::this_thread::yield();
        }
        
        // Procesar el mensaje según su tipo
        if (msg.type == MessageType::READ_RESP || msg.type == MessageType::WRITE_RESP) {
            auto it = pe_registry.find(msg.dest);
            if (it != pe_registry.end()) {
                if (msg.type == MessageType::READ_RESP) {
                    std::cout << "[INTERCONNECT] Enviando respuesta de tipo READ_RESP al PE" << msg.dest << "\n";
                }else{
                    std::cout << "[INTERCONNECT] Enviando respuesta de tipo WRITE_RESP al PE" << msg.dest << "\n";
                }
                
                it->second->receiveResponse(msg);
            } else {
                std::cerr << "[INTERCONNECT] PE destino no registrado: " << msg.dest << "\n";
            }
        } else if (memory) {
            std::cout << "[INTERCONNECT] Mensaje del PE " << msg.src << " procesado... Enviando mensaje a memoria \n";
            memory->receive(msg);
        } else {
            std::cerr << "[INTERCONNECT] Error: memoria no conectada\n";
        }
        auto msg_end = std::chrono::steady_clock::now();
        icstats.total_processing_time += (msg_end - msg_start);
    }

    std::cout << "[INTERCONNECT] Finalizó procesamiento de mensajes.\n";
    icstats.total_operation_time = std::chrono::steady_clock::now() - start_time;
}

// Método para convertir el tipo de mensaje a una cadena de texto
std::string messageTypeToString(MessageType type) {
    switch (type) {
        case MessageType::WRITE_MEM: return "WRITE_MEM";
        case MessageType::READ_MEM: return "READ_MEM";
        case MessageType::BROADCAST_INVALIDATE: return "BROADCAST_INVALIDATE";
        case MessageType::INV_ACK: return "INV_ACK";
        case MessageType::INV_COMPLETE: return "INV_COMPLETE";
        case MessageType::READ_RESP: return "READ_RESP";
        case MessageType::WRITE_RESP: return "WRITE_RESP";
        default: return "UNKNOWN";
    }
}

// Método para imprimir estadísticas del Interconnect
void Interconnect::printStatistics() const {
    std::cout << "\n=== Interconnect Statistics ===\n";
    std::cout << "Total messages processed: " << std::accumulate(
        icstats.message_counts.begin(), icstats.message_counts.end(), 0,
        [](int sum, const auto& pair) { return sum + pair.second; }) << "\n";
        
    std::cout << "Total bytes transferred: " << icstats.total_bytes_transferred << " bytes\n";
    std::cout << "Total processing time: " << icstats.total_processing_time.count() << " seconds\n";
    std::cout << "Total operation time: " << icstats.total_operation_time.count() << " seconds\n";
    
    std::cout << "\nMessage types distribution:\n";
    for (const auto& [type, count] : icstats.message_counts) {
        std::cout << messageTypeToString(type) << ": " << count << "\n";
    }
    
    std::cout << "\nMessages per PE:\n";
    for (const auto& [pe_id, count] : icstats.messages_per_pe) {
        if (pe_id == 0) {
            std::cout << "Memory: " << count << " messages\n";
        } else
        {
            std::cout << "PE" << pe_id << ": " << count << " messages\n";
        }
        
    }
}

// Metodo para obtener el total de mensajes procesados
int Interconnect::getTotalMessages() const {
    int total = 0;
    for (const auto& [type, count] : icstats.message_counts) {
        total += count;
    }
    return total;
}

// Método para obtener el total de bytes transferidos
size_t Interconnect::getTotalBytes() const {
    return icstats.total_bytes_transferred;
}

// Método para obtener el tiempo total de procesamiento
double Interconnect::getProcessingTime() const {
    return icstats.total_processing_time.count();
}
// Método para obtener el conteo de mensajes por tipo
std::map<MessageType, int> Interconnect::getMessageCounts() const {
    return icstats.message_counts;
}

