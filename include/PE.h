#ifndef PE_H
#define PE_H

#include <array>
#include <cstdint>
#include <string>

constexpr size_t CACHE_BLOCKS = 128;
constexpr size_t CACHE_LINE_SIZE = 16;

class PE {
public:
    PE(uint8_t src, uint8_t qos);

    uint8_t getSrcId() const;
    uint8_t getQoS() const;

    void printCacheInfo() const;

private:
    uint8_t src_id;
    uint8_t qos_value;

    // Caché privada: 128 bloques de 16 bytes
    std::array<std::array<uint8_t, CACHE_LINE_SIZE>, CACHE_BLOCKS> cache;
};

#endif // PE_H
