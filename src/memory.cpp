#include "../include/memory.h"
#include <chrono>
#include <thread>
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
            std::cerr << "Mensaje no válido para Memory\n";
            break;
    }
}

void Memory::handle_read(const SMS& msg) {
    std::cout << "[READ_MEM] PE" << static_cast<int>(msg.src)
              << " solicitó leer " << msg.size
              << " bytes desde la dirección " << msg.addr << "\n";

    // Simula tiempo = SIZE * 0.2
    double delay_seconds = msg.size * 0.2;
    std::thread([=]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay_seconds * 1000)));

        SMS response(MessageType::READ_RESP);
        response.dest = msg.src;
        response.qos = msg.qos;
        response.data = std::vector<uint8_t>(msg.size, 0xAB);  // data ficticia

        send_to_interconnect(response);
    }).detach();
}

void Memory::handle_write(const SMS& msg) {
    uint32_t total_bytes = msg.size * 16;

    std::cout << "[WRITE_MEM] PE" << static_cast<int>(msg.src)
              << " solicitó escribir " << msg.size
              << " líneas de caché (" << total_bytes
              << " bytes) desde la línea " << msg.cache_line
              << " a la dirección " << msg.addr << "\n";

    double delay_seconds = total_bytes * 0.2;
    std::thread([=]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay_seconds * 1000)));

        SMS response(MessageType::WRITE_RESP);
        response.dest = msg.src;
        response.qos = msg.qos;
        response.status = 0x1; // escritura exitosa

        send_to_interconnect(response);
    }).detach();
}
