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


constexpr size_t SHARED_MEMORY_SIZE = 4096;   // Posiciones de memoria
constexpr size_t SHARED_MEMORY_WIDTH = 32;    // Ancho de cada posición de memoria compartida
constexpr size_t CACHE_WORD_WIDTH = 16;            // Ancho de cada posición de memoria cache
constexpr size_t MAX_CONCURRENT_MESSAGES = 4; // Cantidad máxima de mensajes que se pueden ejecutar en simultáneo en memoria (quad channel de base)

class Memory {
    // Esta clase representa la memoria compartida en el sistema
    // y se encarga de gestionar las solicitudes de lectura y escritura
    // desde las unidades de procesamiento (PEs).
    // La memoria tiene un tamaño fijo y se accede a través de mensajes SMS.
public:
    // Constructor de la clase Memory
    explicit Memory(std::function<void(const SMS&)> response_callback);
    ~Memory();

    // Establece el callback para enviar respuestas despues de procesar la solicitud del PE
    void setResponseCallback(std::function<void(const SMS&)> cb);

    // Booleano que indica si la memoria está inactiva
    bool isIdle() const { return is_idle.load(); }
 
    // Establece el valor de PENALTY_TIMER y PENALTY_BYTES (delay x bytes)
    void setPenaltyTimers(double new_penalty_timer, double new_penalty_bytes);

    // Se encarga de recibir mensajes de las PEs y los coloca en la cola de entrada
    void receive(const SMS& msg); 

    // Inicia el hilo de gestión de operaciones
    void start();

    // Detiene el hilo de gestión de operaciones
    void stop();

private:
    // Creación de la memoria compartida
    std::array<uint32_t, SHARED_MEMORY_SIZE> memory{};

    // Hilo de gestión de operaciones
    void managerThread();

    // Estructura que representa una operación activa en la memoria
    struct ActiveOperation {
        SMS msg;
        std::chrono::steady_clock::time_point ready_time;
    };

    // Variables de penalización de tiempo
    // PENALTY_TIMER: Tiempo de penalización por operación base
    // PENALTY_BYTES: Tiempo de penalización por byte
    double PENALTY_TIMER = 1;
    double PENALTY_BYTES= 0.2;

    // Variables de estado
    // is_idle: Indica si la memoria está inactiva
    std::atomic<bool> is_idle = true;

    // Cola de mensajes entrantes a memoria.
    std::queue<SMS> incoming_queue;

    // Cola de mensajes que están siendo procesados
    std::vector<ActiveOperation> active_operations;

    // Booleano que indica si la memoria está en ejecución
    std::atomic<bool> running;

    // Condición de variable para la sincronización entre hilos
    std::mutex mutex_;
    std::condition_variable cv; 
    
    // Callback para enviar respuestas a las PEs
    std::function<void(const SMS&)> send_response;

    // Hilo de gestión de operaciones
    std::thread manager;
};

#endif
