#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "include/PE.h"
#include "include/Interconnect.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
     QVector<int> getPEPriorities() const;
     int start_simulation( QVector<int> priorities,QString dir);
     void generatePECSVReport(const std::vector<std::unique_ptr<PE>>& pes);
     const char* messageTypeToString(MessageType type);
     void generateCSVReports(const Interconnect& interconnect, const std::vector<std::unique_ptr<PE>>& pes);


private:
    Ui::MainWindow *ui;

private slots:
    void on_pushButton_clicked();
};


#endif // MAINWINDOW_H
