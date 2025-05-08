#ifndef MEMORY_H
#define MEMORY_H

#include "SMS.h"
#include <queue>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <chrono>



class Memory {
public:
    explicit Memory(std::function<void(const SMS&)> response_callback);
    ~Memory();

    void setResponseCallback(std::function<void(const SMS&)> cb);
    bool isIdle() const { return is_idle.load(); }
 
    void setPenaltyTimers(double new_penalty_timer, double new_penalty_bytes);

    void receive(const SMS& msg); 
    void start();
    void stop();

private:
    void managerThread();

    struct ActiveOperation {
        SMS msg;
        std::chrono::steady_clock::time_point ready_time;
    };

    double PENALTY_TIMER = 1;
    double PENALTY_BYTES= 0.2;

    std::atomic<bool> is_idle = true;
    std::queue<SMS> incoming_queue;
    std::vector<ActiveOperation> active_operations;

    std::mutex mutex_;
    std::condition_variable cv; 
    std::atomic<bool> running;

    std::function<void(const SMS&)> send_response;
    std::thread manager;
};

#endif
