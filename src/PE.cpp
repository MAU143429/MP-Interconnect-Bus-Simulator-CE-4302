#include "../include/PE.h"
#include <iostream>
#include <iomanip>

PE::PE(uint8_t src, uint8_t qos)
    : src_id(src), qos_value(qos) {
        
    // Inicializa toda la caché con ceros
    for (auto& block : cache) {
        block.fill(0);
    }
}

uint8_t PE::getSrcId() const {
    return src_id;
}

uint8_t PE::getQoS() const {
    return qos_value;
}
 
void PE::printCacheInfo() const {
    std::cout << "PE[" << static_cast<int>(src_id)
              << "] - QoS: " << static_cast<int>(qos_value)
              << " - Cache inicilizada [" << CACHE_BLOCKS << " bloques de "
              << CACHE_LINE_SIZE << " bytes]\n";
}
