#include "../include/Interconnect.h"
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

void Interconnect::processQueue() {
    while (running) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        cv.wait(lock, [&]() { return !message_queue.empty() || !running; });

        if (!running) break;

        SMS msg = message_queue.front();
        message_queue.pop();
        lock.unlock();

        // Simular tiempo de transferencia
        double delay_secs = 2.0;
        if (msg.type == MessageType::WRITE_MEM || msg.type == MessageType::READ_RESP) {
            delay_secs += msg.data.size() * 0.1;
        } else if (msg.type == MessageType::READ_MEM || msg.type == MessageType::WRITE_RESP) {
            delay_secs += msg.size * 0.1;
        }

        std::cout << "[INTERCONNECT] Procesando mensaje de PE" << msg.src
                  << " con delay de " << delay_secs << "s\n";
        std::this_thread::sleep_for(std::chrono::duration<double>(delay_secs));

        std::cout << "[INTERCONNECT] Mensaje procesado.\n";
    }
}
