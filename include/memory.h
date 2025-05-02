#ifndef MEMORY_H
#define MEMORY_H

#include "../include/sms.h"
#include <thread>
#include <functional>
#include <iostream>

class Memory {
public:
    // Simula la memoria; puede ser un puntero a una función de envío al interconnect
    explicit Memory(std::function<void(const SMS&)> interconnect_cb);

    // Procesa un mensaje recibido
    void process_message(const SMS& msg);

private:
    std::function<void(const SMS&)> send_to_interconnect;

    void handle_read(const SMS& msg);
    void handle_write(const SMS& msg);
};

#endif
