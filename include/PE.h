// --- PE.h ---
#ifndef PE_H
#define PE_H

#include "SMS.h"
#include <vector>
#include <functional>
#include <atomic>
#include <map>

constexpr size_t MAX_CACHE_BLOCKS = 128; // Tamaño máximo de bloques de caché
constexpr size_t CACHE_BLOCK_WIDTH = 16; // Tamaño de cada bloque de caché en bytes


// Estructura para almacenar estadísticas de cada PE
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

// Clase que representa una unidad de procesamiento (PE)
// Cada PE tiene un ID, una calidad de servicio (QoS) y una lista de instrucciones (SMS)
class PE {
public:
    // Constructor de la clase PE
    PE(int id, int qos, const std::vector<SMS>& instructions);

    // Declaracion que prohibe la copia y asignación de objetos PE
    PE(const PE&) = delete;
    PE& operator=(const PE&) = delete;
    PE(PE&&) noexcept = default;
    PE& operator=(PE&&) noexcept = default;

    // Método que ejecuta las instrucciones de la PE
    // Este método toma una función de callback que envía mensajes al bus de interconexión
    void run(std::function<bool(const SMS&)> send_to_interconnect);

    // Método que recibe la respuesta del bus de interconexión
    void receiveResponse(const SMS& response);
    
    // Método que devuelve el vector de instrucciones de la PE
    const std::vector<SMS>& getInstructionList() const;

    // Método que devuelve el ID de la PE
    int getId() const;
    
    // Método que ejecuta el print de las estadísticas de la PE
    void printStatistics() const;

    // Métodos para la cantidad de instrucciones ejecutadas, respuestas recibidas, bytes enviados, tiempo de espera y tiempo de ejecución
    int getInstructionsExecuted() const;
    int getResponsesReceived() const;
    size_t getBytesSent() const;
    double getWaitTime() const;
    double getExecutionTime() const;
    std::string getCSVLine() const;
    

private:
    // Atributos privados de cada PE
    int id;
    int qos;
    std::vector<SMS> instruction_list;
    size_t current_instruction_index;

    // Cache privada de cada PE
    std::array<uint16_t, MAX_CACHE_BLOCKS> cache;

    // Varibles atomicas para controlar el estado de espera de respuesta
    std::atomic<bool> awaiting_response;

    // Callback para enviar mensajes al bus de interconexión
    std::function<bool(const SMS&)> send_callback;

    // Estructura para almacenar estadísticas de cada PE
    PEStats stats;

};

#endif