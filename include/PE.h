// --- PE.h ---
#ifndef PE_H
#define PE_H

#include "SMS.h"
#include <vector>
#include <functional>
#include <atomic>

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

private:
    int id;
    int qos;
    std::vector<SMS> instruction_list;
    std::atomic<bool> awaiting_response;
    size_t current_instruction_index;
    std::function<bool(const SMS&)> send_callback;

};

#endif