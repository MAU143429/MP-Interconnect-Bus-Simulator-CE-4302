#include "../include/PE.h"
#include <iostream>
#include <thread>

// Constructor de la clase PE
PE::PE(int id, int qos, const std::vector<SMS>& instructions)
    : id(id), qos(qos), instruction_list(instructions),
      awaiting_response(false), current_instruction_index(0) {}



// Funcion que ejecuta las instrucciones de la PE
void PE::run(std::function<bool(const SMS&)> send_to_interconnect) {
    send_callback = send_to_interconnect;

    while (current_instruction_index < instruction_list.size() || awaiting_response) {

        if (!awaiting_response) {

            const SMS& instr = instruction_list[current_instruction_index];
            bool sent = send_to_interconnect(instr);

            if (sent) {
                awaiting_response = true;
                ++current_instruction_index;
            } else {
                std::this_thread::yield();
            }
        } else {
            std::this_thread::yield();
        }
    }

    std::cout << "[PE " << id << "] Finalizó ejecución.\n";
}

// Funcion que recibe la respuesta del bus de interconexión
void PE::receiveResponse(const SMS& response) {
    if (response.dest != id) return;  // Ignorar si no es para este PE

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

// Funcion que devuelve el id de la PE
int PE::getId() const {
    return id;
}

// Funcion que devuelve la lista de instrucciones de la PE
const std::vector<SMS>& PE::getInstructionList() const {
    return instruction_list;
}
