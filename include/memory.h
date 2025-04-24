#ifndef MEMORY_H
#define MEMORY_H

#include <array>
#include <vector>
#include <cstdint>
#include <cstddef>

constexpr size_t MEMORY_SIZE = 4096;  // Palabras (32 bits)
constexpr size_t WORD_SIZE = 4;       // Bytes por palabra

class Memory {
public:
    Memory();

    std::vector<uint8_t> read(uint32_t addr, size_t size) const;
    void write(uint32_t addr, const std::vector<uint8_t>& data);

private:
    std::array<uint32_t, MEMORY_SIZE> memory;
};

#endif // MEMORY_H
