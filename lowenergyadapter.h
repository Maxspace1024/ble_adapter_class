#ifndef LOWENERGYADAPTER_H
#define LOWENERGYADAPTER_H

#include <QObject>
#include <QLowEnergyController>
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceDiscoveryAgent>

class LowEnergyAdapter : public QObject
{
    Q_OBJECT
public:
    //inherit from QObject, has SIGNAL & SLOT method
    explicit LowEnergyAdapter(QObject *parent = nullptr);
    ~LowEnergyAdapter();

    // scanning the peripheral devices
    void startScan(int discoveryTimeout);       // milli-second
    void stopScan();
    void showDevices();

    // connection
    void connectToDevice(int index);
    void disconnectFromDevice();

    // the I/O of Characteristic
    QLowEnergyCharacteristic::PropertyTypes getCharacteristicProperties(const QString &);
    QLowEnergyService* findServiceOfCharacteristic(const QString &);
    QString readCharacteristic(const QString &);
    void writeCharacteristic(const QString &,const QByteArray &);
    void enableCharacteristicNotification(const QString &,bool);
    void enableCharacteristicIndication(const QString &,bool);

    // check the discovery of searching all characteristics in each service
    bool isAllDetailFinished();
private:
    QObject* _parent;

    int detailFinishCount;

    QList<QBluetoothUuid> UUIDServices;
    QList<QLowEnergyService*> Services;
    QList<QBluetoothDeviceInfo> peripheralDevices;

    QLowEnergyController* controller;
    QBluetoothLocalDevice* localDevice;
    QBluetoothDeviceDiscoveryAgent* agent;

    QHash< QString , QLowEnergyService* > UUID2ParentService;
    //QHash< QString , QLowEnergyCharacteristic > UUID2Charact;
private slots:
    void onScanFinish();
    void onScanCancel();
    void onDeviceDiscover(const QBluetoothDeviceInfo &);
    void onAgentErrorOccur(QBluetoothDeviceDiscoveryAgent::Error);

    void onControllerConnect();
    void onControllerDisconnect();
    void onControllerDiscoveryServicesFinish();
    void onControllerErrorOccur(QLowEnergyController::Error);

    void onServiceStateChange(QLowEnergyService::ServiceState);
    void onServiceCharacteristicChange(QLowEnergyCharacteristic,QByteArray);
    void onServiceCharacteristicRead(QLowEnergyCharacteristic,QByteArray);
    void onServiceCharacteristicWritten(QLowEnergyCharacteristic,QByteArray);
signals:

};

#endif // LOWENERGYADAPTER_H
