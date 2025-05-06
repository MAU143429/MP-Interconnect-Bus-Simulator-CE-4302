#include "../include/Memory.h"
#include <iostream>

Memory::Memory(std::function<void(const SMS&)> response_callback)
    : send_response(std::move(response_callback)), running(false) {}

Memory::~Memory() {
    stop();
}

void Memory::start() {
    running = true;
    manager = std::thread(&Memory::managerThread, this);
}

void Memory::stop() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        running = false;
        cv.notify_all();
    }

    if (manager.joinable())
        manager.join();
}


void Memory::receive(const SMS& msg) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        incoming_queue.push(msg);
        std::cout << "[MEMORY] Mensaje recibido de PE" << msg.src << "\n";
    }
    cv.notify_one();
}

void Memory::setResponseCallback(std::function<void(const SMS&)> cb) {
    send_response = std::move(cb);
}


void Memory::managerThread() {
    using namespace std::chrono;

    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv.wait_for(lock, std::chrono::milliseconds(10), [&] {
            return !incoming_queue.empty() || !active_operations.empty() || !running;
        });

        if (!running && incoming_queue.empty() && active_operations.empty())
            break;


        // Mover a activos si hay espacio
        while (!incoming_queue.empty() && active_operations.size() < 4) {
            SMS msg = incoming_queue.front();
            incoming_queue.pop();

            double delay_secs = 0;
            if (msg.type == MessageType::WRITE_MEM)
                delay_secs = msg.num_of_cache_lines * 16 * 0.2;
            else if (msg.type == MessageType::READ_MEM)
                delay_secs = msg.size * 0.2;

            auto now = steady_clock::now();
            auto delay = duration_cast<steady_clock::duration>(duration<double>(delay_secs));
            auto ready_time = now + delay;

            std::cout << "[MEMORY] Iniciando operación de PE" << msg.src
                      << " (delay: " << delay_secs << "s)\n";

            ActiveOperation op;
            op.msg = msg;
            op.ready_time = ready_time;
            active_operations.push_back(op);
        }

        // Verificar operaciones listas
        auto now = steady_clock::now();
        auto it = active_operations.begin();
        while (it != active_operations.end()) {
            if (now >= it->ready_time) {
                SMS& msg = it->msg;

                SMS resp(msg.type == MessageType::READ_MEM ? MessageType::READ_RESP : MessageType::WRITE_RESP);
                resp.dest = msg.src;
                resp.status = 1; // dummy
                resp.data = {42}; // dummy

                std::cout << "[MEMORY] Terminó operación de PE" << msg.src << "\n";
                send_response(resp);
                it = active_operations.erase(it);
                cv.notify_one();  
            } else {
                ++it;
            }
        }
    }
}
