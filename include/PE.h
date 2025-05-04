#ifndef PE_H
#define PE_H

#include <vector>
#include "sms.h"

class PE {
public:
    PE(int src, int qos, const std::vector<SMS>& instrs);

    int getSrcId() const;
    int getQoS() const;
    std::vector<SMS> getInstructions() const;

private:
    int src_id;
    int qos_value;
    std::vector<SMS> instructions;
};

#endif // PE_H
