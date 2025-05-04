#include "../include/Memory.h"
#include <iostream>
#include <vector>
#include <functional>

Memory::Memory(std::function<void(const SMS&)> interconnect_cb)
    : send_to_interconnect(std::move(interconnect_cb)) {}

void Memory::process_message(const SMS& msg) {
    switch (msg.type) {
        case MessageType::READ_MEM:
            handle_read(msg);
            break;
        case MessageType::WRITE_MEM:
            handle_write(msg);
            break;
        default:
            std::cerr << "Mensaje no valido para Memory\n";
            break;
    }
}


void Memory::handle_read(const SMS& msg) {
    std::cout << "[READ_MEM] PE" << msg.src
              << " solicito leer " << msg.size
              << " bytes desde la direccion " << msg.addr << "\n";

    // Simular datos sin delay ni hilos
    SMS response(MessageType::READ_RESP);
    response.dest = msg.src;
    response.qos = msg.qos;
    response.data = std::vector<int>(msg.size, 0xAB);  // datos ficticios con int

    send_to_interconnect(response);
}

void Memory::handle_write(const SMS& msg) {
    int total_bytes = msg.num_of_cache_lines * 16;

    std::cout << "[WRITE_MEM] PE" << msg.src
              << " solicito escribir " << msg.num_of_cache_lines
              << " lineas de cache (" << total_bytes
              << " bytes) desde la linea " << msg.start_cache_line
              << " a la direccion " << msg.addr << "\n";

    SMS response(MessageType::WRITE_RESP);
    response.dest = msg.src;
    response.qos = msg.qos;
    response.status = 1; // escritura exitosa

    send_to_interconnect(response);
}
