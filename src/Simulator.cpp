#include "../include/PE.h"
#include "../include/Parser.h"
#include "../include/Interconnect.h"
#include "../include/Memory.h"

#include <iostream>
#include <memory>
#include <vector>
#include <thread>

int main() {
    /*
    const int NUM_PE = 10;
    std::vector<std::unique_ptr<PE>> pes;
    std::vector<std::thread> pe_threads;

    // Crear e iniciar el interconnect
    Interconnect interconnect;
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

    // Detener el interconnect
    interconnect.stop();

    std::cout << "[SIMULADOR] Todos los PE han terminado.\n";*/

    Memory memory([](const SMS& resp) {
        std::cout << "[RESPUESTA] Enviada a PE" << resp.dest << "\n";
        resp.printInfo();
        std::cout << "-----------------------\n";
    });

    memory.start();

    SMS m1(MessageType::WRITE_MEM); m1.src = 1; m1.num_of_cache_lines = 5;
    SMS m2(MessageType::READ_MEM);  m2.src = 2; m2.size = 64;
    SMS m3(MessageType::READ_MEM);  m3.src = 3; m3.size = 128;
    SMS m4(MessageType::WRITE_MEM); m4.src = 4; m4.num_of_cache_lines = 2;
    SMS m5(MessageType::READ_MEM);  m5.src = 5; m5.size = 512;

    memory.receive(m1);
    memory.receive(m2);
    memory.receive(m3);
    memory.receive(m4);
    memory.receive(m5); // debe quedar esperando

    memory.stop();

    return 0;
}
