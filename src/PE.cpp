#include "../include/PE.h"
#include <iostream>
#include <iomanip>

PE::PE(int src, int qos)
    : src_id(src), qos_value(qos) {

}

int PE::getSrcId() const {
    return src_id;
}

int PE::getQoS() const {
    return qos_value;
}
