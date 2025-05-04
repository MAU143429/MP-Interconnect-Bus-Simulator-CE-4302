#include "../include/PE.h"
#include <iostream>
#include <iomanip>

PE::PE(int src, int qos, const std::vector<SMS>& instrs)
    : src_id(src), qos_value(qos), instructions(instrs) {

}

int PE::getSrcId() const {
    return src_id;
}

int PE::getQoS() const {
    return qos_value;
}

std::vector<SMS> PE::getInstructions() const {
    if (instructions.empty()) {
        std::cout << "No hay instrucciones disponibles." << std::endl;
        return {};
    }
    std::cout << "Instrucciones disponibles:" << std::endl;
    return instructions;
}

