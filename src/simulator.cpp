#include "../include/PE.h"
#include <iostream>
#include <vector>

int main() {

    std::vector<PE> proccesing_elements;

    for (uint8_t i = 0; i < 10; ++i) {
        // AQUI DEBERIA LLAMARSE AL VECTOR DE PRIORIDADES QUE NOS QT 
        uint8_t qos = 0x10 + i; 
        proccesing_elements.emplace_back(i, qos);
    }

    // Mostrar info de cada PE creado
    for (const auto& pe : proccesing_elements) {
        pe.printCacheInfo();
    }

    return 0;
}
