#ifndef SMS_H
#define SMS_H

#include <vector>
#include <iostream>
#include <chrono>

// Definición de los tipos de mensajes
enum class MessageType {
    WRITE_MEM,
    READ_MEM,
    BROADCAST_INVALIDATE,
    INV_ACK,
    INV_COMPLETE,
    READ_RESP,
    WRITE_RESP
};

// Estructura para almacenar estadísticas de mensajes
struct MessageStats {
    size_t size_bytes;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
};

class SMS {
public:

    // Atributos del mensaje
    MessageType type;
    int src;                 // PE que envía el mensaje
    int addr;                // Dirección de memoria
    int size;                // Tamaño en bytes o número de líneas
    int qos;                 // Calidad de servicio

    int dest;                // PE destino (para RESP)
    std::vector<int> data;   // Datos (READ_RESP, WRITE_MEM)
    int num_of_cache_lines;  // Líneas de caché a escribir (WRITE_MEM)
    int start_cache_line;    // Línea de caché inicial (WRITE_MEM)
    int inv_cache_line;      // Línea de caché (BROADCAST_INVALIDATE)
    int status;              // STATUS en WRITE_RESP

    SMS(MessageType t);      // Constructor con tipo de mensaje
    SMS();                   // constructor vacio

    // Metodo para imprimir la información del mensaje
    void printInfo() const;

    // Método para calcular el tamaño del mensaje en bytes
    size_t calculateSize() const;
};

#endif
