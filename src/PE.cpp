#include "../include/PE.h"
#include <iostream>
#include <thread>

// Constructor de la clase PE
PE::PE(int id, int qos, const std::vector<SMS>& instructions)
    : id(id), qos(qos), instruction_list(instructions),
      awaiting_response(false), current_instruction_index(0) {}



// Funcion que ejecuta las instrucciones de la PE
void PE::run(std::function<bool(const SMS&)> send_to_interconnect) {
    while (current_instruction_index < instruction_list.size()) {
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
    if (response.dest == id) {
        awaiting_response = false;
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
