#include "Memory.h"
#include <iostream>
#include <vector>
#include <functional>

using namespace std::chrono;

Memory::Memory(std::function<void(const SMS&)> interconnect_cb)
    : send_to_interconnect(std::move(interconnect_cb)) {}

void Memory::process_message(const SMS& msg) {
    request_queue.push(msg);
}

void Memory::begin_processing_next() {
    if (request_queue.empty()) return;

    current_msg = request_queue.front();
    request_queue.pop();

    int delay_ms = 0;
    if (current_msg.type == MessageType::READ_MEM) {
        delay_ms = static_cast<int>(current_msg.size * 200);  // 0.2s por byte → 200ms
        std::cout << "[MEMORY] [READ_MEM] PE" << current_msg.src
                  << " pidió leer " << current_msg.size << " bytes\n";
    } else if (current_msg.type == MessageType::WRITE_MEM) {
        int total_bytes = current_msg.num_of_cache_lines * 16;
        delay_ms = static_cast<int>(total_bytes * 200);  // 0.2s por byte
        std::cout << "[MEMORY] [WRITE_MEM] PE" << current_msg.src
                  << " pidió escribir " << total_bytes << " bytes\n";
    }

    ready_time = steady_clock::now() + milliseconds(delay_ms);
    is_busy = true;
}

void Memory::update() {
    if (is_busy) {
        if (steady_clock::now() >= ready_time) {
            if (current_msg.type == MessageType::READ_MEM) {
                SMS response(MessageType::READ_RESP);
                response.dest = current_msg.src;
                response.qos = current_msg.qos;
                response.data = std::vector<int>(current_msg.size, 0xAB);
                send_to_interconnect(response);
            } else if (current_msg.type == MessageType::WRITE_MEM) {
                SMS response(MessageType::WRITE_RESP);
                response.dest = current_msg.src;
                response.qos = current_msg.qos;
                response.status = 1;
                send_to_interconnect(response);
            }
            is_busy = false;
        }
    }

    if (!is_busy) {
        begin_processing_next();
    }
}
