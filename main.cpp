#include "mainwindow.h"

#include <QApplication>

#include "include/PE.h"
#include "include/Parser.h"
#include "include/Interconnect.h"
#include "include/Memory.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <memory>
#include <vector>
#include <thread>


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();


    // Inicializar el número de PEs y crear los vectores para almacenar PEs y sus hilos
    

    return a.exec();
}
