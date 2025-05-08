#include "../include/PE.h"
#include "../include/sms.h"
#include "../include/memory.h"
#include "../include/parser.h"
#include "../include/GlobalClock.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>



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
    
    const int NUM_PE = 10;
    std::vector<PE> pes;

    for (int pe_id = 1; pe_id < NUM_PE + 1; ++pe_id) {
        std::string filename = "../data/Workload_1/PE" + std::to_string(pe_id) + ".txt";
        std::vector<SMS> instrs = parseInstructionsFromFile(filename);
        
        int qos_dummy = 0; 
        pes.emplace_back(pe_id, qos_dummy, instrs);
    }

    /* 
    std::vector<SMS> printinst = pes[0].getInstructions();
    std::cout << printinst.size() << " instrucciones disponibles.\n";

    for (const auto& msg : printinst) {
        msg.printInfo();
    }*/
    
    // Crear clase Memory con el callback


    Memory memory(send_to_interconnect);

    // Crear mensaje tipo WRITE_MEM
    SMS write_sms(MessageType::WRITE_MEM);
    write_sms.src = 3;
    write_sms.addr = 0x8000;
    write_sms.num_of_cache_lines = 2;
    write_sms.start_cache_line = 5;
    write_sms.qos = 0x15;

    // Procesar mensaje con la clase Memory
    memory.process_message(write_sms);

    // Crear mensaje tipo READ_MEM
    SMS read_sms(MessageType::READ_MEM);
    read_sms.src = 2;
    read_sms.addr = 0x18AF;
    read_sms.size = 4;
    read_sms.qos = 0x15;

    // Procesar mensaje con la clase Memory
    memory.process_message(read_sms);

    
    // Loop de simulación en tiempo real
    const int MAX_SECONDS = 10;
    auto start = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(MAX_SECONDS)) {
        memory.update();  
        std::this_thread::yield();  // Ceder el hilo para evitar un bucle apretado
    }

    std::cout << "Simulación terminada\n";

    
    return 0;
    
}
