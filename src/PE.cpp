#include "../include/PE.h"
#include <iostream>
#include <thread>
#include <fstream>
#include <sstream>


// Constructor de la clase PE
PE::PE(int id, int qos, const std::vector<SMS>& instructions)
    : id(id), qos(qos), instruction_list(instructions),
      awaiting_response(false), current_instruction_index(0) {}



// Funcion que ejecuta las instrucciones de la PE
void PE::run(std::function<bool(const SMS&)> send_to_interconnect) {
    
    // Guardar el callback de envío
    send_callback = send_to_interconnect;

    auto start_time = std::chrono::steady_clock::now();

    // Verificar si la lista de instrucciones está vacía o esta esperando respuesta del bus de interconexión
    while (current_instruction_index < instruction_list.size() || awaiting_response) {

        if (!awaiting_response) {

            // Enviar la instrucción actual al bus de interconexión
            const SMS& instr = instruction_list[current_instruction_index];

            stats.recordSentMessage(instr);
            stats.instructions_executed++;

            bool sent = send_to_interconnect(instr);

            if (sent) {
                awaiting_response = true;
                ++current_instruction_index;
            } else {
                auto wait_start = std::chrono::steady_clock::now();
                std::this_thread::yield();
                //stats.total_wait_time += (std::chrono::steady_clock::now() - wait_start);
                if (std::chrono::steady_clock::now() - wait_start >= std::chrono::milliseconds(1)) {
                    stats.total_wait_time += (std::chrono::steady_clock::now() - wait_start);
                    wait_start = std::chrono::steady_clock::now(); // Reiniciar contador
                }
            }
        } else {
            auto wait_start = std::chrono::steady_clock::now();
            std::this_thread::yield();
            //stats.total_wait_time += (std::chrono::steady_clock::now() - wait_start);
            if (std::chrono::steady_clock::now() - wait_start >= std::chrono::milliseconds(1)) {
                stats.total_wait_time += (std::chrono::steady_clock::now() - wait_start);
                wait_start = std::chrono::steady_clock::now(); // Reiniciar contador
            }
        }
    }
    stats.total_execution_time = std::chrono::steady_clock::now() - start_time;
    std::cout << "[PE " << id << "] Finalizó ejecución.\n";
}

// Funcion que recibe la respuesta del bus de interconexión
void PE::receiveResponse(const SMS& response) {

    // Verificar si la respuesta es para este PE
    if (response.dest != id) return;  // Ignorar si no es para este PE
    stats.responses_received++;

    // Imprimir información de la respuesta
    switch (response.type) {
        case MessageType::READ_RESP: { 
            std::cout << "[PE" << id << "] READ_RESP recibida.\n";
            awaiting_response = false;
            break;
        }
        case MessageType::WRITE_RESP:{
            std::cout << "[PE" << id << "] WRITE_RESP recibida.\n";
            awaiting_response = false;
            break;
        }case MessageType::INV_COMPLETE: {
            awaiting_response = false;
            break;
        }

        case MessageType::BROADCAST_INVALIDATE: {
            std::cout << "[PE" << id << "] Recibido BROADCAST_INVALIDATE. Enviando INV_ACK.\n";

            // Enviar INV_ACK al bus de interconexión para confirmar la invalidación
            SMS ack;
            ack.type = MessageType::INV_ACK;
            ack.src = id;
            ack.qos = response.qos;

            if (send_callback) {
                send_callback(ack);
            }
            break;
        }

        default: {
            std::cout << "[PE" << id << "] Mensaje desconocido recibido.\n";
            break;
        }
    }
}

// Funcion que convierte el tipo de mensaje a una cadena de texto
// Esta función es útil para imprimir estadísticas y depurar
const char* MessageTypeToString(MessageType type) {
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

// Funcion que imprime las estadisticas de la PE al finalizar la ejecucion
void PE::printStatistics() const {
    std::cout << "\n=== PE " << id << " Statistics ===\n";
    std::cout << "Instructions executed: " << stats.instructions_executed << "\n";
    std::cout << "Responses received: " << stats.responses_received << "\n";
    std::cout << "Total execution time: " << stats.total_execution_time.count() << " seconds\n";
    std::cout << "Total wait time: " << stats.total_wait_time.count() << " seconds (" 
              << (stats.total_wait_time.count() / stats.total_execution_time.count() * 100) << "%)\n";
    std::cout << "Total bytes sent: " << stats.total_bytes_sent << " bytes\n";
    
    std::cout << "\nSent messages by type:\n";
    for (const auto& [type, count] : stats.sent_messages) {
        std::cout << MessageTypeToString(type) << ": " << count << " messages ("
                  << (count * 100.0 / stats.instructions_executed) << "%)\n";
    }
}

// Funcion que convierte las estadisticas de la PE a una cadena de texto en formato CSV
std::string PE::getCSVLine() const {
    std::ostringstream csv_line;
    csv_line << id << ","
             << stats.instructions_executed << ","
             << stats.responses_received << ","
             << stats.total_bytes_sent << ","
             << stats.total_wait_time.count() << ","
             << stats.total_execution_time.count();
    
    // Agregar conteo por tipo de mensaje
    for (const auto& [type, count] : stats.sent_messages) {
        csv_line << "," << count;
    }
    
    return csv_line.str();
}

// Funcion que devuelve el id de la PE
int PE::getId() const {
    return id;
}

// Funcion que devuelve la lista de instrucciones de la PE
const std::vector<SMS>& PE::getInstructionList() const {
    return instruction_list;
}

// Funcion que devuelve la cantidad de instrucciones ejecutadas
int PE::getInstructionsExecuted() const {
    return stats.instructions_executed;
}

// Funcion que devuelve la cantidad de respuestas recibidas
int PE::getResponsesReceived() const {
    return stats.responses_received;
}

// Funcion que devuelve la cantidad de bytes enviados
size_t PE::getBytesSent() const {
    return stats.total_bytes_sent;
}

// Funcion que devuelve el tiempo de espera
double PE::getWaitTime() const {
    return stats.total_wait_time.count();
}

// Funcion que devuelve el tiempo de ejecucion
double PE::getExecutionTime() const {
    return stats.total_execution_time.count();
}
