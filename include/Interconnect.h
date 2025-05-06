#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include "SMS.h"
#include <queue>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>

class Interconnect {
public:
    Interconnect();
    ~Interconnect();

    bool receiveMessage(const SMS& msg); // Llamado por los PE
    void setMemory(class Memory* mem);
    void start();
    void stop();

private:
    void processQueue(); // Función del hilo

    struct PendingMessage {
        SMS msg;
        std::chrono::steady_clock::time_point ready_time;
    };
    Memory* memory = nullptr;
    std::queue<SMS> message_queue;
    std::vector<PendingMessage> pending;
    std::mutex queue_mutex;
    std::condition_variable cv;
    std::thread processing_thread;
    std::atomic<bool> running;
};

#endif // INTERCONNECT_H
