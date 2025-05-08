#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include "SMS.h"
#include "PE.h"
#include <queue>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <functional>
#include <optional>

class Interconnect {
public:
    Interconnect();
    ~Interconnect();

    bool receiveMessage(const SMS& msg); // Llamado por los PE
    void setSchedulingMode(bool fifo); // true = FIFO, false = QoS

    
    void setMemory(class Memory* mem);
    void start();
    void stop();

    void registerPE(int id, PE* pe);


private:
    void processQueue(); // Función del hilo
    void wait_until(std::chrono::steady_clock::time_point ready_time); 

    struct PendingMessage {
        SMS msg;
        std::chrono::steady_clock::time_point ready_time;
    };

    struct InvalidationState {
        int origin_id;                  // PE que generó el broadcast
        int expected_acks;              // Cantidad total esperada (n - 1)
        int received_acks = 0;          // Contador
        MessageType waiting_type;      // Solo BROADCAST_INVALIDATE por ahora
        SMS original_msg;              // Para recuperar datos de qos o line
    };
    
    bool fifo_mode = true; // true = FIFO, false = QoS-based
    Memory* memory = nullptr;
    //std::queue<SMS> message_queue;
    std::deque<SMS> message_queue;

    std::vector<PendingMessage> pending;
    std::unordered_map<int, PE*> pe_registry;
    std::queue<SMS> invalidation_queue;   // Cola para mensajes INV_ACK
    std::optional<InvalidationState> current_invalidation;



    std::mutex queue_mutex;
    std::condition_variable cv;
    std::thread processing_thread;
    std::atomic<bool> running;
};

#endif // INTERCONNECT_H
