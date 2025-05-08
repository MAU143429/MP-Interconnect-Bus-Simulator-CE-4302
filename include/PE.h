// --- PE.h ---
#ifndef PE_H
#define PE_H

#include "SMS.h"
#include <vector>
#include <functional>
#include <atomic>
#include <map>

struct PEStats {
    int instructions_executed = 0;
    int responses_received = 0;
    std::map<MessageType, int> sent_messages;
    std::chrono::duration<double> total_wait_time;
    std::chrono::duration<double> total_execution_time;
    size_t total_bytes_sent = 0;
    
    // Método para registrar un mensaje enviado
    void recordSentMessage(const SMS& msg) {
        sent_messages[msg.type]++;
        total_bytes_sent += msg.calculateSize();
    }
};

class PE {
public:
    PE(int id, int qos, const std::vector<SMS>& instructions);

    PE(const PE&) = delete;
    PE& operator=(const PE&) = delete;
    PE(PE&&) noexcept = default;
    PE& operator=(PE&&) noexcept = default;

    void run(std::function<bool(const SMS&)> send_to_interconnect);
    void receiveResponse(const SMS& response);
    

    int getId() const;
    const std::vector<SMS>& getInstructionList() const;

    void printStatistics() const;

    int getInstructionsExecuted() const;
    int getResponsesReceived() const;
    size_t getBytesSent() const;
    double getWaitTime() const;
    double getExecutionTime() const;
    std::string getCSVLine() const;
    

private:
    int id;
    int qos;
    std::vector<SMS> instruction_list;
    std::atomic<bool> awaiting_response;
    size_t current_instruction_index;
    std::function<bool(const SMS&)> send_callback;
    PEStats stats;
    


};

#endif