#ifndef MEMORY_H
#define MEMORY_H

#include "../include/SMS.h"
#include "../include/Event.h"
#include <thread>
#include <functional>
#include <iostream>
#include <queue>

class Memory {
public:
    // Simula la memoria; puede ser un puntero a una función de envío al interconnect
    explicit Memory(std::function<void(const SMS&)> interconnect_cb);
    void tick();
    void process_message(const SMS& msg);

private:
    std::function<void(const SMS&)> send_to_interconnect;
    std::queue<SMS> request_queue; 
    void begin_processing_next();   
    bool is_busy = false;
    int ready_tick = 0;
    SMS current_msg;  
};

#endif
