#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
     int start_simulation( QVector<int> priorities);


private:
    Ui::MainWindow *ui;

private slots:
    void on_pushButton_clicked();
};


#endif // MAINWINDOW_H
