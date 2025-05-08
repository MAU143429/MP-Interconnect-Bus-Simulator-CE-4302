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

int MainWindow::start_simulation( QVector<int> priorities,QString dir){

    qDebug() << "Priorities:" << priorities;
    const int NUM_PE = 10;
    std::vector<std::unique_ptr<PE>> pes;
    std::vector<std::thread> pe_threads;

    // Crear e iniciar Interconnect
    Interconnect interconnect;


    interconnect.setSchedulingMode(false); // false = usar modo QoS


    // Crear memoria y arrancarla
    Memory memory([&interconnect](const SMS& resp) {
        std::cout << "[MEMORY] Respuesta generada para el PE" << resp.dest <<  " enviando al Interconnect \n";
        interconnect.receiveMessage(resp);

    });

    memory.start();


    interconnect.setMemory(&memory);
    interconnect.start();

    // Cargar instrucciones y crear PEs
    for (int pe_id = 1; pe_id <= NUM_PE; ++pe_id) {
        std::string filename = dir.toStdString()+"/PE" + std::to_string(pe_id) + ".txt";
        std::vector<SMS> instrs = parseInstructionsFromFile(filename);

        auto pe = std::make_unique<PE>(pe_id, priorities[pe_id-1], instrs);


        interconnect.registerPE(pe_id, pe.get());

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

    // Esperar a que todos los PE terminen
    for (auto& t : pe_threads) {
        t.join();
    }

    pes.clear();  // Limpiar el vector de PEs

    interconnect.stop();

    memory.stop();
    return 0;
}

