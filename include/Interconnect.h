    #ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include "SMS.h"
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

class Interconnect {
public:
    Interconnect();
    ~Interconnect();

    bool receiveMessage(const SMS& msg); // Llamado por los PE
    void start();
    void stop();

private:
    void processQueue(); // Función del hilo

    std::queue<SMS> message_queue;
    std::mutex queue_mutex;
    std::condition_variable cv;
    std::thread processing_thread;
    std::atomic<bool> running;
};

#endif // INTERCONNECT_H
