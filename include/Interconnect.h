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

class Interconnect {
public:
    Interconnect();
    ~Interconnect();

    bool receiveMessage(const SMS& msg); // Llamado por los PE
    void setMemory(class Memory* mem);
    void start();
    void stop();

    void registerPE(int id, PE* pe);


private:
    void processQueue(); // Función del hilo

    struct PendingMessage {
        SMS msg;
        std::chrono::steady_clock::time_point ready_time;
    };

    Memory* memory = nullptr;
    std::queue<SMS> message_queue;
    std::vector<PendingMessage> pending;
    std::unordered_map<int, PE*> pe_registry;


    std::mutex queue_mutex;
    std::condition_variable cv;
    std::thread processing_thread;
    std::atomic<bool> running;
};

#endif // INTERCONNECT_H
