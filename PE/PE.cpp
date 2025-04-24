#include "PE.h"
#include <iostream>

PE::PE(uint8_t id, uint8_t qos)
    : src_id(id), qos_value(qos) {
    for (auto& block : cache) {
        block.fill(0);
    }
}

void PE::loadInstruction(const Message& instr) {
    std::lock_guard<std::mutex> lock(pe_mutex);
    instruction_memory.push(instr);
}

bool PE::hasPendingInstructions() const {
    return !instruction_memory.empty();
}

Message PE::fetchNextMessage() {
    std::lock_guard<std::mutex> lock(pe_mutex);
    if (instruction_memory.empty()) {
        throw std::runtime_error("No instruction to fetch.");
    }
    Message msg = instruction_memory.front();
    instruction_memory.pop();
    return msg;
}

void PE::step() {
    if (!hasPendingInstructions()) return;

    Message msg = fetchNextMessage();

    std::cout << "PE[" << static_cast<int>(src_id) << "] ejecutando: ";
    switch (msg.type) {
        case MessageType::WRITE_MEM:
            std::cout << "WRITE_MEM a " << msg.addr << std::endl;
            break;
        case MessageType::READ_MEM:
            std::cout << "READ_MEM de " << msg.addr << " tamaño " << msg.size << std::endl;
            break;
        case MessageType::BROADCAST_INVALIDATE:
            std::cout << "BROADCAST_INVALIDATE línea " << msg.addr << std::endl;
            break;
        default:
            std::cout << "Otro mensaje" << std::endl;
    }
}
