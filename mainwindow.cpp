#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "include/PE.h"
#include "include/Parser.h"
#include "include/Interconnect.h"
#include "include/Memory.h"
#include "ui_mainwindow.h"
#include <QtConcurrent>

#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <QFileDialog>
#include <QString>
#include <iostream>
#include <fstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
QVector<int> MainWindow::getPEPriorities() const {
    return {
        ui->PE1_prty->value(),
        ui->PE2_prty->value(),
        ui->PE3_prty->value(),
        ui->PE4_prty->value(),
        ui->PE5_prty->value(),
        ui->PE6_prty->value(),
        ui->PE7_prty->value(),
        ui->PE8_prty->value(),
        ui->PE9_prty->value(),
        ui->PE10_prty->value()
    };
}

void MainWindow::on_pushButton_clicked()
{
    QVector<int> priorities = this->getPEPriorities();
    ui->pages->setCurrentIndex(1);
    QString dir = QFileDialog::getExistingDirectory(this, "Seleccione directorio del Workload");
    if (dir.isEmpty())
        return;

    QFuture<void> future = QtConcurrent::run([=]() {
        this->start_simulation(priorities, dir);
    });
}


// Función para convertir el tipo de mensaje a una cadena
const char* MainWindow::messageTypeToString(MessageType type) {
    switch(type) {
    case MessageType::WRITE_MEM: return "WRITE_MEM";
    case MessageType::READ_MEM: return "READ_MEM";
    case MessageType::BROADCAST_INVALIDATE: return "BROADCAST_INVALIDATE";
    case MessageType::INV_ACK: return "INV_ACK";
    case MessageType::INV_COMPLETE: return "INV_COMPLETE";
    case MessageType::READ_RESP: return "READ_RESP";
    case MessageType::WRITE_RESP: return "WRITE_RESP";
    default: return "UNKNOWN";
    }
}

// Función para generar el reporte CSV de los PEs
void MainWindow::generatePECSVReport(const std::vector<std::unique_ptr<PE>>& pes) {
    std::ofstream csv_file("pe_stats.csv");

    // Encabezados
    csv_file << "PE_ID,InstructionsExecuted,ResponsesReceived,BytesSent,WaitTimeSec,ExecTimeSec";

    // Encabezados dinámicos para tipos de mensaje
    const char* message_types[] = {
        "WRITE_MEM", "READ_MEM", "BROADCAST_INVALIDATE",
        "INV_ACK", "INV_COMPLETE", "READ_RESP", "WRITE_RESP"
    };

    for (const auto& type : message_types) {
        csv_file << "," << type << "_Count";
    }

    csv_file << "\n";

    // Datos de cada PE
    for (const auto& pe : pes) {
        csv_file << pe->getCSVLine() << "\n";
    }

    csv_file.close();
    std::cout << "[SIMULATOR] Reporte CSV generado: pe_stats.csv\n";
}

// Función para generar los reportes CSV del Interconnect y de los PEs
void MainWindow::generateCSVReports(const Interconnect& interconnect, const std::vector<std::unique_ptr<PE>>& pes) {
    // 1. Reporte del Interconnect
    std::ofstream ic_csv("interconnect_report.csv");
    ic_csv << "Metric,Value\n";
    ic_csv << "Total Messages Processed," << interconnect.getTotalMessages() << "\n";
    ic_csv << "Total Bytes Transferred," << interconnect.getTotalBytes() << "\n";
    ic_csv << "Total Processing Time (sec)," << interconnect.getProcessingTime() << "\n";

    // Mensajes por tipo
    auto msg_counts = interconnect.getMessageCounts();
    for (const auto& [type, count] : msg_counts) {
        ic_csv << messageTypeToString(type) << " Messages," << count << "\n";
    }
    ic_csv.close();

    // 2. Reporte de los PEs
    std::ofstream pe_csv("pe_report.csv");
    pe_csv << "PE ID,Instructions Executed,Responses Received,Bytes Sent,Wait Time (sec),Execution Time (sec)\n";

    for (const auto& pe : pes) {
        pe_csv << pe->getId() << ","
               << pe->getInstructionsExecuted() << ","
               << pe->getResponsesReceived() << ","
               << pe->getBytesSent() << ","
               << pe->getWaitTime() << ","
               << pe->getExecutionTime() << "\n";
    }
    pe_csv.close();

    std::cout << "\n[SIMULATOR] Reportes CSV generados:\n";
    std::cout << " - interconnect_report.csv\n";
    std::cout << " - pe_report.csv\n";
}


int MainWindow::start_simulation( QVector<int> priorities,QString dir){

    qDebug() << "Priorities:" << priorities;


    // Inicializar el número de PEs y crear los vectores para almacenar PEs y sus hilos
    const int NUM_PE = 10;
    std::vector<std::unique_ptr<PE>> pes;
    std::vector<std::thread> pe_threads;

    // Inicializar el tiempo de penalización en milisegundos
    const int BYTE_PENALTY = 100; // Penalización en bytes en milisegundos

    // Crear e iniciar Interconnect
    Interconnect interconnect;


    // Se establece el modo de programación FIFO o QoS y se elige el modo ejecución
    interconnect.setSchedulingMode(false); // false = usar modo QoS
    interconnect.setSteppingMode(false);  // Activa el modo stepping

    // Establecer los tiempos de penalización en milisegundos
    interconnect.setPenaltyTimers(200, BYTE_PENALTY);
    //                           base, penalidad en milisegundos

    // Crear memoria y arrancarla
    Memory memory([&interconnect](const SMS& resp) {
        std::cout << "[MEMORY] Respuesta generada para el PE" << resp.dest <<  " enviando al Interconnect \n";
        interconnect.receiveMessage(resp);

    });

    // Establecer los tiempos de penalización en memoria en segundos
    memory.setPenaltyTimers(1,(BYTE_PENALTY/1000));
    //                      base, penalidad en segundos

    // Iniciar el hilo de gestión de operaciones de memoria
    memory.start();

    // Conectar la memoria al Interconnect
    interconnect.setMemory(&memory);

    // Iniciar el Interconnect
    interconnect.start();

    // Cargar instrucciones y crear PEs
    for (int pe_id = 1; pe_id <= NUM_PE; ++pe_id) {
        std::string filename = dir.toStdString()+"/PE" + std::to_string(pe_id) + ".txt";
        std::vector<SMS> instrs = parseInstructionsFromFile(filename, priorities[pe_id-1]); // Cargar instrucciones desde el archivo

        if(instrs.empty()) {
            std::cerr << "Error: No instructions for PE " << pe_id << std::endl;
            continue;
        }

        auto pe = std::make_unique<PE>(pe_id, priorities[pe_id-1], instrs);

        interconnect.registerPE(pe_id, pe.get()); // Registrar el PE en el Interconnect

        pes.push_back(std::move(pe));
    }

    // Lanzar hilos para cada PE
    for (auto& pe : pes) {
        pe_threads.emplace_back([&pe, &interconnect]() {
            pe->run([&interconnect](const SMS& msg) {
                return interconnect.receiveMessage(msg);
            });
        });
    }

    // Hilo para el modo stepping
    std::thread stepping_thread([&interconnect]() {
        std::string input;
        while (true) {
            std::getline(std::cin, input);

            // Si ya terminó, salir
            if (!interconnect.isRunning()) break;

            if (input == "exit") break;

            interconnect.triggerNextStep();
        }
    });

    // Esperar a que todos los PE terminen
    for (auto& t : pe_threads) {
        t.join();

    }

    // Imprimir estadísticas de cada PE
    for (const auto& pe : pes) {
        pe->printStatistics();
    }


    // Generar reportes CSV
    generateCSVReports(interconnect, pes);
    generatePECSVReport(pes);

    // Limpiar los PEs
    pes.clear();

    // Detener el Interconnect a
    interconnect.stop();

    // Detener el hilo de stepping si es necesario
    if (stepping_thread.joinable()) {
        std::cout << "[STEPPING] Todos los PEs han terminado. Presiona ENTER para salir.\n";
        std::cin.get();
        stepping_thread.join();
    }

    // Detener la memoria
    memory.stop();

    // Imprimir estadísticas del Interconnect
    interconnect.printStatistics();

    // Generar gráficos de los reportes CSV utilizando Python
    system("python3 generate_graphs.py");

    return 0;


}

