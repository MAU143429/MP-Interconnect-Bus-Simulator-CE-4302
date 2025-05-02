#ifndef SMS_H
#define SMS_H

#include <vector>
#include <iostream>

enum class MessageType {
    WRITE_MEM,
    READ_MEM,
    BROADCAST_INVALIDATE,
    INV_ACK,
    INV_COMPLETE,
    READ_RESP,
    WRITE_RESP
};

class SMS {
public:
    MessageType type;
    int src;              // PE que envía el mensaje
    int addr;             // Dirección de memoria
    int size;             // Tamaño en bytes o número de líneas
    int qos;              // Calidad de servicio

    int dest;                     // PE destino (para RESP)
    std::vector<int> data;       // Datos (READ_RESP, WRITE_MEM)
    int cache_line;              // Línea de caché (BROADCAST_INVALIDATE)
    int status;                  // STATUS en WRITE_RESP

    SMS(MessageType t);          // Constructor

    void printInfo() const;
};

#endif
