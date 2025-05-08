#include "../include/PE.h"
#include "../include/Parser.h"
#include "../include/Interconnect.h"
#include "../include/Memory.h"

#include <iostream>
#include <memory>
#include <vector>
#include <thread>

int main() {
    // Inicializar el número de PEs y crear los vectores para almacenar PEs y sus hilos
    const int NUM_PE = 10;
    std::vector<std::unique_ptr<PE>> pes;
    std::vector<std::thread> pe_threads;

    // Crear e iniciar Interconnect
    Interconnect interconnect;


    interconnect.setSchedulingMode(false); // false = usar modo QoS


    // Crear memoria y arrancarla
    Memory memory([&interconnect](const SMS& resp) {
        std::cout << "[MEMORY] Respuesta generada para el PE" << resp.dest <<  " enviando al Interconnect \n";
        interconnect.receiveMessage(resp);

    });
    
    memory.start();

    
    interconnect.setMemory(&memory); 
    interconnect.start();

    // Cargar instrucciones y crear PEs
    for (int pe_id = 1; pe_id <= NUM_PE; ++pe_id) {
        std::string filename = "data/Workload_1/PE" + std::to_string(pe_id) + ".txt";
        std::vector<SMS> instrs = parseInstructionsFromFile(filename);
    
        int qos_dummy = 0;
        auto pe = std::make_unique<PE>(pe_id, qos_dummy, instrs);

    
        interconnect.registerPE(pe_id, pe.get());

        pes.push_back(std::move(pe));
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

    pes.clear();  // Limpiar el vector de PEs

    interconnect.stop();

    memory.stop();

    
    return 0;
}
