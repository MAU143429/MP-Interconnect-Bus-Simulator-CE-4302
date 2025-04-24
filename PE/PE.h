#ifndef PE_H
#define PE_H

#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <queue>
#include <mutex>

constexpr size_t CACHE_BLOCKS = 128;
constexpr size_t CACHE_LINE_SIZE = 16;
constexpr size_t INSTRUCTION_MEM_SIZE = 256;

enum class MessageType {
    WRITE_MEM,
    READ_MEM,
    BROADCAST_INVALIDATE,
    INV_ACK,
    INV_COMPLETE,
    READ_RESP,
    WRITE_RESP
};

struct Message {
    MessageType type;
    uint8_t src;
    uint32_t addr;
    uint32_t size;
    uint8_t qos;
    std::vector<uint8_t> data; // Opcional según el tipo
};

class PE {
public:
    PE(uint8_t id, uint8_t qos);

    void loadInstruction(const Message& instr);
    void step(); // Ejecuta una instrucción
    bool hasPendingInstructions() const;

    uint8_t getId() const { return src_id; }
    uint8_t getQoS() const { return qos_value; }

    Message fetchNextMessage();

private:
    uint8_t src_id;
    uint8_t qos_value;

    std::queue<Message> instruction_memory;
    std::array<std::array<uint8_t, CACHE_LINE_SIZE>, CACHE_BLOCKS> cache;

    std::mutex pe_mutex;
};

#endif // PE_H
