#include "../include/PE.h"
#include "../include/Parser.h"
#include "../include/Interconnect.h"
#include "../include/Memory.h"

#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <fstream>

const char* messageTypeToString(MessageType type) {
    switch(type) {
        case MessageType::WRITE_MEM: return "WRITE_MEM";
        case MessageType::READ_MEM: return "READ_MEM";
        case MessageType::BROADCAST_INVALIDATE: return "BROADCAST_INVALIDATE";
        case MessageType::INV_ACK: return "INV_ACK";
        case MessageType::INV_COMPLETE: return "INV_COMPLETE";
        case MessageType::READ_RESP: return "READ_RESP";
        case MessageType::WRITE_RESP: return "WRITE_RESP";
        default: return "UNKNOWN";
    }
}

void generatePECSVReport(const std::vector<std::unique_ptr<PE>>& pes) {
    std::ofstream csv_file("pe_stats.csv");
    
    // Encabezados
    csv_file << "PE_ID,InstructionsExecuted,ResponsesReceived,BytesSent,WaitTimeSec,ExecTimeSec";
    
    // Encabezados dinámicos para tipos de mensaje
    const char* message_types[] = {
        "WRITE_MEM", "READ_MEM", "BROADCAST_INVALIDATE", 
        "INV_ACK", "INV_COMPLETE", "READ_RESP", "WRITE_RESP"
    };
    
    for (const auto& type : message_types) {
        csv_file << "," << type << "_Count";
    }
    
    csv_file << "\n";
    
    // Datos de cada PE
    for (const auto& pe : pes) {
        csv_file << pe->getCSVLine() << "\n";
    }
    
    csv_file.close();
    std::cout << "[SIMULATOR] Reporte CSV generado: pe_stats.csv\n";
}

void generateCSVReports(const Interconnect& interconnect, const std::vector<std::unique_ptr<PE>>& pes) {
    // 1. Reporte del Interconnect
    std::ofstream ic_csv("interconnect_report.csv");
    ic_csv << "Metric,Value\n";
    ic_csv << "Total Messages Processed," << interconnect.getTotalMessages() << "\n";
    ic_csv << "Total Bytes Transferred," << interconnect.getTotalBytes() << "\n";
    ic_csv << "Total Processing Time (sec)," << interconnect.getProcessingTime() << "\n";
    
    // Mensajes por tipo
    auto msg_counts = interconnect.getMessageCounts();
    for (const auto& [type, count] : msg_counts) {
        ic_csv << messageTypeToString(type) << " Messages," << count << "\n";
    }
    ic_csv.close();

    // 2. Reporte de los PEs
    std::ofstream pe_csv("pe_report.csv");
    pe_csv << "PE ID,Instructions Executed,Responses Received,Bytes Sent,Wait Time (sec),Execution Time (sec)\n";
    
    for (const auto& pe : pes) {
        pe_csv << pe->getId() << ","
               << pe->getInstructionsExecuted() << ","
               << pe->getResponsesReceived() << ","
               << pe->getBytesSent() << ","
               << pe->getWaitTime() << ","
               << pe->getExecutionTime() << "\n";
    }
    pe_csv.close();

    std::cout << "\n[SIMULATOR] Reportes CSV generados:\n";
    std::cout << " - interconnect_report.csv\n";
    std::cout << " - pe_report.csv\n";
}

int main() {
    // Inicializar el número de PEs y crear los vectores para almacenar PEs y sus hilos
    const int NUM_PE = 10;
    const int BYTE_PENALTY = 100; // Penalización en bytes en milisegundos
    std::vector<std::unique_ptr<PE>> pes;
    std::vector<std::thread> pe_threads;

    // Crear e iniciar Interconnect
    Interconnect interconnect;


    interconnect.setSchedulingMode(true); // false = usar modo QoS

    interconnect.setPenaltyTimers(200, BYTE_PENALTY); // Establecer los tiempos de penalización en milisegundos
    //                           base, penalidad en milisegundos                             

    // Crear memoria y arrancarla
    Memory memory([&interconnect](const SMS& resp) {
        std::cout << "[MEMORY] Respuesta generada para el PE" << resp.dest <<  " enviando al Interconnect \n";
        interconnect.receiveMessage(resp);

    });

    memory.setPenaltyTimers(1,(BYTE_PENALTY/1000)); // Establecer los tiempos de penalización en memoria en segundos 
    //                      base, penalidad en segundos
    memory.start();

    
    interconnect.setMemory(&memory); 
    interconnect.start();

    // Cargar instrucciones y crear PEs
    for (int pe_id = 1; pe_id <= NUM_PE; ++pe_id) {
        std::string filename = "data/Workload_1/PE" + std::to_string(pe_id) + ".txt";
        std::vector<SMS> instrs = parseInstructionsFromFile(filename);
    
        int qos_dummy = 0;

        if(instrs.empty()) {
            std::cerr << "Error: No instructions for PE " << pe_id << std::endl;
            continue;
        }

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

    for (const auto& pe : pes) {
        pe->printStatistics();
    }

    generateCSVReports(interconnect, pes);
    generatePECSVReport(pes);

    pes.clear();  // Limpiar el vector de PEs

    interconnect.stop();

    memory.stop();

    interconnect.printStatistics();


    system("python3 generate_graphs.py");

    return 0;
}


