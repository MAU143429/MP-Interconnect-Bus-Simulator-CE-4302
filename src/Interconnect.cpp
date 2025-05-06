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
    while (running) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        cv.wait(lock, [&]() {
            return !message_queue.empty() || !running;
        });

        if (!running || message_queue.empty()) continue;

        SMS msg = message_queue.front();
        message_queue.pop();
        lock.unlock();

        // Calcular tiempo de transferencia simulado
        double delay_secs = 2.0;
        if (msg.type == MessageType::WRITE_MEM) {
            delay_secs += msg.num_of_cache_lines * 16 * 0.1;
        } else if (msg.type == MessageType::READ_RESP) {
            delay_secs += msg.data.size() * 0.1;
        } else if (msg.type == MessageType::READ_MEM || msg.type == MessageType::WRITE_RESP) {
            delay_secs += msg.size * 0.1;
        }

        std::cout << "[INTERCONNECT] Procesando mensaje de PE" << msg.src
                  << " con delay de " << delay_secs << "s\n";

        // Simular espera sin bloquear CPU
        auto ready_time = std::chrono::steady_clock::now() + std::chrono::duration<double>(delay_secs);
        while (std::chrono::steady_clock::now() < ready_time) {
            std::this_thread::yield();
        }

        std::cout << "[INTERCONNECT] Mensaje procesado.\n";

        if (memory) {
            std::cout << "[INTERCONNECT] Enviando mensaje a memoria\n";
            memory->receive(msg);
        } else {
            std::cerr << "[INTERCONNECT] Error: memoria no conectada\n";
        }
    }

    std::cout << "[INTERCONNECT] Finalizó procesamiento de mensajes.\n";
}

