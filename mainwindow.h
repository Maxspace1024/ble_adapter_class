#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "lowenergyadapter.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    LowEnergyAdapter* adapter;

private slots:
    void on_startBtn_clicked();

    void on_stopBtn_clicked();

    void on_showDevBtn_clicked();

    void on_connectBtn_clicked();

    void on_disconnectBtn_clicked();

    void on_findBtn_clicked();

    void on_writeBtn_clicked();

    void on_readBtn_clicked();

    void on_propertiesBtn_clicked();

    void on_notifEnableBtn_clicked();

    void on_notifDisableBtn_clicked();

    void onAgentChChange(QLowEnergyService*,const QLowEnergyCharacteristic &);
private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
