#include "../include/memory.h"
#include <stdexcept>
#include <cstring>

Memory::Memory() {
    memory.fill(0);
}

std::vector<uint8_t> Memory::read(uint32_t addr, size_t size) const {
    if (addr + size > MEMORY_SIZE * WORD_SIZE) {
        throw std::out_of_range("Read out of bounds");
    }

    std::vector<uint8_t> result(size);
    const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(memory.data());
    std::memcpy(result.data(), byte_ptr + addr, size);
    return result;
}

void Memory::write(uint32_t addr, const std::vector<uint8_t>& data) {
    if (addr + data.size() > MEMORY_SIZE * WORD_SIZE) {
        throw std::out_of_range("Write out of bounds");
    }

    uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(memory.data());
    std::memcpy(byte_ptr + addr, data.data(), data.size());
}
