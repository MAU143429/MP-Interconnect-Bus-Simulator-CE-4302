#include "../include/Memory.h"
#include "../include/GlobalClock.h"
#include <iostream>
#include <vector>
#include <functional>

Memory::Memory(std::function<void(const SMS&)> interconnect_cb)
    : send_to_interconnect(std::move(interconnect_cb)) {}


void Memory::process_message(const SMS& msg) {
    request_queue.push(msg);
}

void Memory::begin_processing_next() {
    if (request_queue.empty()) return;

    current_msg = request_queue.front();
    request_queue.pop();

    int delay = 0;
    if (current_msg.type == MessageType::READ_MEM) {
        delay = current_msg.size; // 1 tick por byte
        std::cout << "[Tick " << GlobalClock::now() << "] [READ_MEM] PE" << current_msg.src
                  << " pidio leer " << current_msg.size << " bytes\n";
    } else if (current_msg.type == MessageType::WRITE_MEM) {
        int total_bytes = current_msg.num_of_cache_lines * 16;
        delay = total_bytes;
        std::cout << "[Tick " << GlobalClock::now() << "] [WRITE_MEM] PE" << current_msg.src
                  << " pidio escribir " << total_bytes << " bytes\n";
    }

    is_busy = true;
    ready_tick = GlobalClock::now() + delay;
}
void Memory::tick() {
    if (is_busy) {
        if (GlobalClock::now() >= ready_tick) {
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



