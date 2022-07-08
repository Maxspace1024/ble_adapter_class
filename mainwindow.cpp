#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    adapter = new LowEnergyAdapter(this);
    connect(
        adapter,
        SIGNAL(adapterCharacteristicChange(QLowEnergyService*,const QLowEnergyCharacteristic &)),
        this,
        SLOT(onAgentChChange(QLowEnergyService*,const QLowEnergyCharacteristic &))
    );
}

MainWindow::~MainWindow()
{
    delete adapter;     //delete before ui
    delete ui;
}


void MainWindow::on_startBtn_clicked()
{
    adapter->startScan(10000);
}


void MainWindow::on_stopBtn_clicked()
{
    adapter->stopScan();
}


void MainWindow::on_showDevBtn_clicked()
{
    adapter->showDevices();
}


void MainWindow::on_connectBtn_clicked()
{
    int index = ui->spinBox->value();
    adapter->connectToDevice(index);
}


void MainWindow::on_disconnectBtn_clicked()
{
    adapter->disconnectFromDevice();
}


void MainWindow::on_findBtn_clicked()
{
    QString x = ui->uuidLE->text();
    QLowEnergyService* serv = adapter->findServiceOfCharacteristic(x);
    if(serv==nullptr)
        qDebug() << "[FIINSV]\tno service";
    else
        qDebug() << "[FIINSV]\t" << serv->serviceUuid();
}


void MainWindow::on_writeBtn_clicked()
{
    adapter->writeCharacteristic(
        ui->uuidLE->text(),
        QByteArray::fromHex( ui->byteArrLE->text().toLatin1() ),
        6//need .toLatin1()
    );
}


void MainWindow::on_readBtn_clicked()
{
    QByteArray val =
    adapter->readCharacteristic(
        ui->uuidLE->text()
    );

    ui->RvalueLabel->setText("Rvalue:" + val.toHex(' '));
}


void MainWindow::on_propertiesBtn_clicked()
{
    qDebug() << adapter->getCharacteristicProperties( ui->uuidLE->text() );
}


void MainWindow::on_notifEnableBtn_clicked()
{
    adapter->enableCharacteristicNotification(
        ui->uuidLE->text(),
        true
    );
}


void MainWindow::on_notifDisableBtn_clicked()
{
    adapter->enableCharacteristicNotification(
        ui->uuidLE->text(),
        false
    );
}

void MainWindow::onAgentChChange(QLowEnergyService* serv,const QLowEnergyCharacteristic &c)
{
    qDebug() << "[SIGNAL_CATCH]\n"
             << "serv:\t" << serv->serviceUuid().toString() << "\n"
             << "charac:\t"<< c.uuid().toString()
             << c.value().toHex() << "\n"
             << "\n";
}
