#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include "SMS.h"
#include <queue>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <functional>

class Interconnect {
public:
    Interconnect();
    ~Interconnect();

    bool receiveMessage(const SMS& msg); // Llamado por los PE
    void setMemory(class Memory* mem);
    void start();
    void stop();

    // NUEVOS MÉTODOS
    void registerPE(int pe_id, std::function<void(const SMS&)> callback);
    void receiveFromMemory(const SMS& msg); // llamado por memoria

private:
    void processQueue(); // Función del hilo

    struct PendingMessage {
        SMS msg;
        std::chrono::steady_clock::time_point ready_time;
    };

    Memory* memory = nullptr;
    std::queue<SMS> message_queue;
    std::vector<PendingMessage> pending;

    std::unordered_map<int, std::function<void(const SMS&)>> pe_callbacks;

    std::mutex queue_mutex;
    std::condition_variable cv;
    std::thread processing_thread;
    std::atomic<bool> running;
};

#endif // INTERCONNECT_H
