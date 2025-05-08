#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include "SMS.h"
#include "PE.h"
#include <queue>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <functional>
#include <optional>
#include <map>

constexpr int CACHE_WIDTH = 16; // Tamaño de la línea de caché en bytes


// Estructura para almacenar estadísticas del Interconnect
struct InterconnectStats {
    std::map<MessageType, int> message_counts;
    size_t total_bytes_transferred;
    std::chrono::duration<double> total_processing_time;
    std::chrono::duration<double> total_operation_time;
    std::map<int, int> messages_per_pe; // PE ID -> count
};

// Clase Interconnect
// Esta clase representa el bus de interconexión entre las PEs y la memoria
// y se encarga de gestionar la comunicación entre ellas.
class Interconnect {
public:

    // Cosntructor y destructor de la clase Interconnect
    Interconnect();
    ~Interconnect();

    // Metodo para recibir mensajes de las PEs
    bool receiveMessage(const SMS& msg); 

    // Método para establecer el modo de calendarización
    void setSchedulingMode(bool fifo); // true = FIFO, false = QoS

    // Método para establecer el tiempo de penalización base y el tiempo de penalización por byte
    void setPenaltyTimers(double new_penalty_timer, double new_penalty_bytes);
    
    // Método para establecer el modo de stepping
    void setSteppingMode(bool enabled);

    // Método para obtener el modo de stepping
    bool getSteppingMode();

    // Método que sirve como trigger para el modo stepping
    void triggerNextStep();

    // Booleano que indica si el Interconnect está en ejecución
    bool isRunning() const;

    // Método para establecer la memoria
    void setMemory(class Memory* mem);

    // Método para iniciar el Interconnect
    void start();

    // Método para detener el Interconnect
    void stop();

    // Método para registrar cada PE
    void registerPE(int id, PE* pe);

    // Método para imprimir estadísticas
    void printStatistics() const;

    // Getters para las estadísticas
    int getTotalMessages() const;
    size_t getTotalBytes() const;
    double getProcessingTime() const;
    std::map<MessageType, int> getMessageCounts() const;

private:
    // Método para procesar los mensajes en la cola principal
    void processQueue(); 

    // Metodo para simular el tiempo de espera
    void wait_until(std::chrono::steady_clock::time_point ready_time); 

    // Estructura que representa un mensaje pendiente
    struct PendingMessage {
        SMS msg;
        std::chrono::steady_clock::time_point ready_time;
    };

    // Estructura que representa el estado de invalidación
    struct InvalidationState {
        int origin_id;                  // PE que generó el broadcast
        int expected_acks;              // Cantidad total esperada (n - 1)
        int received_acks = 0;          // Contador
        MessageType waiting_type;       // Solo BROADCAST_INVALIDATE por ahora
        SMS original_msg;               // Para recuperar datos de qos o line
    };
    
    // Booleano que indica si el Interconnect está en modo FIFO o QoS
    bool fifo_mode = true; // true = FIFO, false = QoS-based

    // Punter a la memoria
    Memory* memory = nullptr;
    

    // Variables de penalización de tiempo
    int PENALTY_TIMER = 200;
    int PENALTY_BYTES= 100;

    // Cola de mensajes principal
    std::deque<SMS> message_queue;


    // Variables para el control de invalidaciones
    std::vector<PendingMessage> pending;
    std::unordered_map<int, PE*> pe_registry;
    std::queue<SMS> invalidation_queue;  
    std::optional<InvalidationState> current_invalidation;
    

    // Variables de control para modo stepping
    bool stepping_mode = false;
    bool step_ready = false;
    std::mutex step_mutex;
    std::condition_variable step_cv;


    // Booleano que indica si el Interconnect está en ejecución
    std::atomic<bool> running;

    // Instancia del hilo de procesamiento
    std::thread processing_thread;

    // Variables de control de acceso a la cola
    std::mutex queue_mutex;
    std::condition_variable cv;
    
    
};

#endif // INTERCONNECT_H
