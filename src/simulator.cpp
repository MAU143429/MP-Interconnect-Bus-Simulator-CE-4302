#include "../include/PE.h"
#include "../include/sms.h"
#include "../include/memory.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

// Callback simulado para enviar mensajes al interconnect
void send_to_interconnect(const SMS& msg) {
    std::cout << "[INTERCONNECT] Recibio mensaje tipo ";
    switch (msg.type) {
        case MessageType::READ_RESP:
            std::cout << "READ_RESP\n";
            break;
        case MessageType::WRITE_RESP:
            std::cout << "WRITE_RESP con status = " << msg.status << "\n";
            break;
        default:
            std::cout << "otro tipo no esperado\n";
            break;
    }
}

int main() {
    std::vector<PE> processing_elements;

    for (int i = 0; i < 8; ++i) {
        int qos = 0x10 + i;
        processing_elements.emplace_back(i, qos);
    }

    for (const auto& pe : processing_elements) {
        pe.printCacheInfo();
    }

    // Crear clase Memory con el callback
    Memory memory(send_to_interconnect);

    // Crear mensaje tipo WRITE_MEM
    SMS write_sms(MessageType::WRITE_MEM);
    write_sms.src = 3;
    write_sms.addr = 0x8000;
    write_sms.size = 4;
    write_sms.qos = 0x15;
    write_sms.cache_line = 5;
    write_sms.dest = 7;
    write_sms.status = 1;

    // Simular datos de escritura
    write_sms.data = {10, 20, 30, 40};

    // Mostrar el contenido del mensaje
    std::cout << "\n=== Informacion del mensaje WRITE_MEM ===\n";
    write_sms.printInfo();
    std::cout << "========================================\n\n";

    // Procesar mensaje con la clase Memory
    memory.process_message(write_sms);


    // Esperar 2 segundos
    for (volatile int i = 0; i < 200000000; ++i) {
        // Busy-wait loop to simulate a delay
    }

    // Crear mensaje tipo READ_MEM
    SMS read_sms(MessageType::READ_MEM);
    read_sms.src = 3;
    read_sms.addr = 0x18AF;
    read_sms.size = 4;
    read_sms.qos = 0x15;
    read_sms.cache_line = 5;
    read_sms.dest = 7;

    // Mostrar el contenido del mensaje
    std::cout << "\n=== Informacion del mensaje READ_MEM ===\n";
    read_sms.printInfo();
    std::cout << "========================================\n\n";

    // Procesar mensaje con la clase Memory
    memory.process_message(read_sms);


    // No se requiere pausa ya que no hay hilos simulados

    return 0;
}
