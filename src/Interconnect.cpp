#include "../include/Interconnect.h"
#include "../include/Memory.h"
#include <iostream>
#include <chrono>
#include <numeric>



InterconnectStats icstats;


Interconnect::Interconnect() : running(false) {}

Interconnect::~Interconnect() {
    stop();
}


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


void Interconnect::start() {
    running = true;
    processing_thread = std::thread(&Interconnect::processQueue, this);
}

void Interconnect::stop() {
    running = false;
    cv.notify_all();
    if (processing_thread.joinable()) {
        processing_thread.join();
    }
}

void Interconnect::setMemory(Memory* mem) {
    memory = mem;
}

void Interconnect::registerPE(int id, PE* pe) {
    pe_registry[id] = pe;
}

void Interconnect::setSchedulingMode(bool fifo) {
    fifo_mode = fifo;
}


void Interconnect::wait_until(std::chrono::steady_clock::time_point ready_time) {
    while (std::chrono::steady_clock::now() < ready_time) {
        std::this_thread::yield();
    }
}

// Modifica el valor de PENALTY_TIMER
void Interconnect::setPenaltyTimers(double new_penalty_timer, double new_penalty_bytes) {
    PENALTY_TIMER = new_penalty_timer;
    PENALTY_BYTES = new_penalty_bytes;
}


void Interconnect::processQueue() {
    auto start_time = std::chrono::steady_clock::now();
    
    while (running || !message_queue.empty() || (memory && !memory->isIdle())) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        cv.wait(lock, [&]() {
            return !message_queue.empty() || !running;
        });

        //if (!running || message_queue.empty()) continue;

        if (message_queue.empty()) {
            std::this_thread::yield();
            continue;
        }

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

        lock.unlock();

        // Antes de procesar cada mensaje:
        auto msg_start = std::chrono::steady_clock::now();
        icstats.message_counts[msg.type]++;
        icstats.messages_per_pe[msg.src]++;
        icstats.total_bytes_transferred += msg.calculateSize();

        // Si es BROADCAST_INVALIDATE, manejar toda la lógica especial
        if (msg.type == MessageType::BROADCAST_INVALIDATE) {
            std::cout << "[INTERCONNECT] Procesando BROADCAST_INVALIDATE de PE" << msg.src << " con delay de 0,1s \n";

            wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(PENALTY_BYTES));

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

            // Esperar y procesar los INV_ACK con delay de 0.2s cada uno
            while (current_invalidation->received_acks < current_invalidation->expected_acks) {
                while (invalidation_queue.empty()) {
                    std::this_thread::yield();
                }

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

        // Procesamiento normal para otros mensajes
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

        

        auto ready_time = std::chrono::steady_clock::now() + std::chrono::duration<double>(delay_secs);
        while (std::chrono::steady_clock::now() < ready_time) {
            std::this_thread::yield();
        }

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

int Interconnect::getTotalMessages() const {
    int total = 0;
    for (const auto& [type, count] : icstats.message_counts) {
        total += count;
    }
    return total;
}

size_t Interconnect::getTotalBytes() const {
    return icstats.total_bytes_transferred;
}

double Interconnect::getProcessingTime() const {
    return icstats.total_processing_time.count();
}

std::map<MessageType, int> Interconnect::getMessageCounts() const {
    return icstats.message_counts;
}

