#include "../include/PE.h"
#include "../include/Parser.h"
#include "../include/Interconnect.h"
#include "../include/Memory.h"

#include <iostream>
#include <memory>
#include <vector>
#include <thread>

int main() {
    
    const int NUM_PE = 10;
    std::vector<std::unique_ptr<PE>> pes;
    std::vector<std::thread> pe_threads;

    // Crear memoria y arrancarla
    Memory memory([](const SMS& resp) {
        std::cout << "[RESPUESTA] Enviada a PE" << resp.dest << "\n";
        resp.printInfo();
        std::cout << "-----------------------\n";
    });
    memory.start();

    // Crear e iniciar Interconnect
    Interconnect interconnect;
    interconnect.setMemory(&memory);  // Conectar memoria
    interconnect.start();

    // Cargar instrucciones y crear PEs
    for (int pe_id = 1; pe_id <= NUM_PE; ++pe_id) {
        std::string filename = "data/Workload_1/PE" + std::to_string(pe_id) + ".txt";
        std::vector<SMS> instrs = parseInstructionsFromFile(filename);

        int qos_dummy = 0;
        pes.push_back(std::make_unique<PE>(pe_id, qos_dummy, instrs));
    }

    // Lanzar hilos para cada PE
    for (auto& pe : pes) {
        pe_threads.emplace_back([&pe, &interconnect]() {
            pe->run([&interconnect](const SMS& msg) {
                return interconnect.receiveMessage(msg);
            });
        });
    }

    // Esperar a que todos los PE terminen
    for (auto& t : pe_threads) {
        t.join();
    }

    interconnect.stop();

    memory.stop();

    return 0;
}
