#include "../include/Memory.h"
#include <iostream>

// Constructor de la clase Memory
Memory::Memory(std::function<void(const SMS&)> response_callback)
    : send_response(std::move(response_callback)), running(false) {}

// Destructor de la clase Memory
Memory::~Memory() {
    stop();
}

// Inicia el hilo de gestión de operaciones
void Memory::start() {
    running = true;
    manager = std::thread(&Memory::managerThread, this);
}

// Detiene el hilo de gestión de operaciones
void Memory::stop() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        running = false;
        cv.notify_all();
    }

    if (manager.joinable())
        manager.join();
}

// Recibe un mensaje de una PE y lo coloca en la cola de entrada
void Memory::receive(const SMS& msg) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        incoming_queue.push(msg);
        is_idle = false; 
        std::cout << "[MEMORY] Mensaje recibido de PE" << msg.src << " esperando a ser procesado...\n";
    }
    cv.notify_one();
}

// Establece el callback para enviar respuestas despues de procesar la solicitud del PE
void Memory::setResponseCallback(std::function<void(const SMS&)> cb) {
    send_response = std::move(cb);
}


// Modifica el valor de PENALTY_TIMER
void Memory::setPenaltyTimers(double new_penalty_timer, double new_penalty_bytes) {
    PENALTY_TIMER = new_penalty_timer;
    PENALTY_BYTES = new_penalty_bytes;
}

// Hilo de gestión de operaciones
// Este hilo se encarga de procesar las solicitudes de memoria y enviar respuestas
void Memory::managerThread() {
    using namespace std::chrono; 

    while (true) {

        // Esperar hasta que haya mensajes en la cola de entrada o que haya operaciones activas
        std::unique_lock<std::mutex> lock(mutex_);
        cv.wait_for(lock, std::chrono::milliseconds(10), [&] {
            return !incoming_queue.empty() || !active_operations.empty() || !running;
        });

        // Si no hay más mensajes, no esta en running y no hay operaciones activas, salir del bucle
        if (!running && incoming_queue.empty() && active_operations.empty()){
            is_idle = true;
            break;
        }

        // Mover a activos si hay espacio maximo 4 operaciones al mismo tiempo (quad channel)

        while (!incoming_queue.empty() && active_operations.size() < MAX_CONCURRENT_MESSAGES) {
            
            // Tomar el mensaje de la cola de entrada
            SMS msg = incoming_queue.front();
            incoming_queue.pop();

            // Calculo del tiempo de espera para la operación
            double delay_secs = 0;
            if (msg.type == MessageType::WRITE_MEM)
                delay_secs = PENALTY_TIMER + ( msg.num_of_cache_lines * CACHE_WORD_WIDTH * PENALTY_BYTES );
            else if (msg.type == MessageType::READ_MEM)
                delay_secs = PENALTY_TIMER + msg.size * PENALTY_BYTES;

            // Calcular el tiempo de finalización de la operación
            auto now = steady_clock::now();
            auto delay = duration_cast<steady_clock::duration>(duration<double>(delay_secs));
            auto ready_time = now + delay;

            std::cout << "[MEMORY] Iniciando operacion de PE" << msg.src
                      << " duracion esperada: delay-> " << delay_secs << "secs\n";


            // Agregar la operación a la lista de operaciones activas
            ActiveOperation op;
            op.msg = msg;
            op.ready_time = ready_time;
            active_operations.push_back(op);
        }

        // Verificar si hay operaciones activas que han terminado
        auto now = steady_clock::now();
        auto it = active_operations.begin();
        while (it != active_operations.end()) {

            // Si la operación ha terminado, enviar respuesta
            if (now >= it->ready_time) {
                SMS& msg = it->msg;

                SMS resp(msg.type == MessageType::READ_MEM ? MessageType::READ_RESP : MessageType::WRITE_RESP);
                resp.dest = msg.src;
                resp.status = 1; 
                resp.data = {}; 
                resp.size = msg.size;

                std::cout << "[MEMORY] Termino solicitud de PE" << msg.src << ", generando respuesta...\n";
                send_response(resp);
                it = active_operations.erase(it);
                cv.notify_one();  
            } else {
                ++it;
            }
        }

        // Si no hay más mensajes en la cola de entrada y no hay operaciones activas, marcar como inactiva
        is_idle = incoming_queue.empty() && active_operations.empty();

    }
}
