#ifndef MEMORY_H
#define MEMORY_H

#include "../include/SMS.h"
#include <functional>
#include <iostream>
#include <queue>
#include <chrono>  // <-- necesario

class Memory {
public:
    explicit Memory(std::function<void(const SMS&)> interconnect_cb);
    void update();  // cambia "tick" a "update" (más semántico en tiempo real)
    void process_message(const SMS& msg);

private:
    std::function<void(const SMS&)> send_to_interconnect;
    std::queue<SMS> request_queue; 
    void begin_processing_next();   

    bool is_busy = false;
    std::chrono::steady_clock::time_point ready_time;  // <-- reemplaza ready_tick
    SMS current_msg;  
};

#endif
