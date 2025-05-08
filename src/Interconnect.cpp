#include "../include/Interconnect.h"
#include "../include/Memory.h"
#include <iostream>
#include <chrono>

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
    }

    std::cout << "[INTERCONNECT] Finalizó procesamiento de mensajes.\n";
}

