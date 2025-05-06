#include "../include/Interconnect.h"
#include "../include/Memory.h"
#include <iostream>
#include <chrono>

Interconnect::Interconnect() : running(false) {}

Interconnect::~Interconnect() {
    stop();
}

bool Interconnect::receiveMessage(const SMS& msg) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        message_queue.push(msg);
        std::cout << "[INTERCONNECT] Mensaje recibido de PE" << msg.src << "\n";
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

void Interconnect::processQueue() {
    while (running || !message_queue.empty() || !pending.empty()) {
        std::unique_lock<std::mutex> lock(queue_mutex);

        cv.wait_for(lock, std::chrono::milliseconds(10), [&]() {
            return !message_queue.empty() || !pending.empty() || !running;
        });

        auto now = std::chrono::steady_clock::now();

        // Revisar si algún mensaje pendiente ya está listo
        for (auto it = pending.begin(); it != pending.end(); ) {
            if (now >= it->ready_time) {
                std::cout << "[INTERCONNECT] Procesando mensaje de PE"
                          << it->msg.src << " (retraso completado)\n";
                std::cout << "[INTERCONNECT] Mensaje procesado.\n";

                if (memory) {
                    std::cout << "[INTERCONNECT] Enviando mensaje a memoria\n";
                    memory->receive(it->msg);
                } else {
                    std::cerr << "[INTERCONNECT] Error: memoria no conectada\n";
                }
                it = pending.erase(it);
            } else {
                ++it;
            }
        }

        // Si hay mensajes nuevos, calcular su tiempo y pasarlos a `pending`
        while (!message_queue.empty()) {
            SMS msg = message_queue.front();
            message_queue.pop();

            double delay_secs = 2.0;
            if (msg.type == MessageType::WRITE_MEM) {
                delay_secs += msg.num_of_cache_lines * 16 * 0.1;
            } else if (msg.type == MessageType::READ_RESP) {
                delay_secs += msg.data.size() * 0.1;
            } else if (msg.type == MessageType::READ_MEM || msg.type == MessageType::WRITE_RESP) {
                delay_secs += msg.size * 0.1;
            }

            auto ready_time = std::chrono::steady_clock::now() +
                              std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                  std::chrono::duration<double>(delay_secs));

            pending.push_back(PendingMessage{msg, ready_time});

            std::cout << "[INTERCONNECT] PE" << msg.src
                      << " programado para " << delay_secs << "s\n";
        }
    }

    std::cout << "[INTERCONNECT] Finalizó procesamiento de mensajes.\n";
}
