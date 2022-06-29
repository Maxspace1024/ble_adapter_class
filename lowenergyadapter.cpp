#include "lowenergyadapter.h"

LowEnergyAdapter::LowEnergyAdapter(QObject *parent) : QObject(parent)
{
    _parent = parent;
    detailFinishCount=0;

    controller = nullptr;

    agent = new QBluetoothDeviceDiscoveryAgent(parent);
    connect(agent,SIGNAL(finished()),this,SLOT(onScanFinish()));
    connect(agent,SIGNAL(canceled()),this,SLOT(onScanCancel()));
    connect(agent,
            SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
            this,
            SLOT(onDeviceDiscover(QBluetoothDeviceInfo)));
    connect(agent,
            SIGNAL(errorOccurred(QBluetoothDeviceDiscoveryAgent::Error)),
            this,
            SLOT(onAgentErrorOccur(QBluetoothDeviceDiscoveryAgent::Error))
    );

    localDevice = new QBluetoothLocalDevice(this);
}
LowEnergyAdapter::~LowEnergyAdapter()
{
    delete agent;
    delete localDevice;
}

// PUBLIC

void LowEnergyAdapter::startScan(int discoveryTimeout)
{
    agent->setLowEnergyDiscoveryTimeout(discoveryTimeout);

    if(!agent->isActive())
        agent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    else
        qDebug() << "[AGENT]\tscanning";
}
void LowEnergyAdapter::stopScan()
{
    agent->stop();
}
void LowEnergyAdapter::showDevices()
{
    if(!agent->isActive())
    {
        int i=0;
        qDebug() << "=======devices=======";
        for(auto &d : peripheralDevices)
            qDebug() << i++ << " " << d.name();
        qDebug() << "\n";
    }
    else
        qDebug() << "agent is active";
}

void LowEnergyAdapter::connectToDevice(int index)
{
    detailFinishCount=0;

    if(0<= index && index < peripheralDevices.size() && controller == nullptr)
    {
        controller = QLowEnergyController::createCentral
        (
            peripheralDevices.at(index),
            localDevice->address(),
            _parent
        );
        connect(controller,SIGNAL(connected()),this,SLOT(onControllerConnect()));
        connect(controller,SIGNAL(disconnected()),this,SLOT(onControllerDisconnect()));
        connect(controller,SIGNAL(discoveryFinished()),this,SLOT(onControllerDiscoveryServicesFinish()));
        connect(controller,
                SIGNAL(errorOccurred(QLowEnergyController::Error)),
                this,
                SLOT(onControllerErrorOccur(QLowEnergyController::Error))
        );
        controller->connectToDevice();
    }
    else
    {
        qDebug() << "[AGENT]\tthe selected peripheral device index is out of range or double connect\n"
                 << QString("index:%1 [0,%2)ctrller ptr:%3")
                    .arg(
                        QString::number(index),
                        QString::number(peripheralDevices.size()),
                        QString::number((quintptr)controller,16)
                    )
                 << "\n";
    }
}
void LowEnergyAdapter::disconnectFromDevice()
{
    if( isAllDetailFinished() && controller!=nullptr )
    {
        //hash
        UUID2ParentService.clear();
        //UUID2Charact.clear();

        for(auto &s : Services)
        {
            s->disconnect();
            delete s;
        }
        //list
        Services.clear();
        UUIDServices.clear();

        controller->disconnectFromDevice();             //disconnect from peripheral device
        controller->disconnect();                       //disconnect SIGNALS AND SLOTS
        delete controller;                              //free memory
        controller = nullptr;
    }
    else
        qDebug() << "[AGENT]\tdisconnect fail 'cause not all detail has been discover"
                 << detailFinishCount << "/" << Services.size();
}

QLowEnergyService* LowEnergyAdapter::findServiceOfCharacteristic(const QString &uuid)
{
    /*
    for(auto& x : UUID2ParentService.keys())
        qDebug() << x;
    */

    if(UUID2ParentService.contains(uuid))
        return UUID2ParentService[uuid];
    else
        return nullptr;
}

QString LowEnergyAdapter::readCharacteristic(const QString &uuid)
{
    QLowEnergyService* serv = findServiceOfCharacteristic(uuid);

    for(auto& c : serv->characteristics())
    {
        if(c.uuid().toString(QUuid::WithoutBraces) == uuid)
        {
            serv->readCharacteristic(c);

            return c.value().toHex(' ');
        }
        else
        {
            qDebug() << "[READch]\tcharacteristic not found";
            return "";
        }
    }

}
void LowEnergyAdapter::writeCharacteristic(const QString &uuid,const QByteArray &ba)
{
    QLowEnergyService* serv = findServiceOfCharacteristic(uuid);

    for(auto& c : serv->characteristics())
    {
        if(c.uuid().toString(QUuid::WithoutBraces) == uuid)
        {
            if( ba.size() == c.value().size())
                serv->writeCharacteristic(c,ba);
            else
                qDebug() << "[WRITEch]\tarray size does not match";
        }
    }
}


// SLOTS    AGENT
//
void LowEnergyAdapter::onScanFinish()
{
    qDebug() << "[AGENT]\tscan finished";
    peripheralDevices = agent->discoveredDevices();
}
void LowEnergyAdapter::onScanCancel()
{
    qDebug() << "[AGENT]\tscan canceled";
    peripheralDevices = agent->discoveredDevices();
}
void LowEnergyAdapter::onDeviceDiscover(const QBluetoothDeviceInfo &info)
{
    qDebug() << "[AGENT]\tcatch:"
             << info.name();
}
void LowEnergyAdapter::onAgentErrorOccur(QBluetoothDeviceDiscoveryAgent::Error error)
{
    qDebug() << "[AGNET]\terror:" << error;
}

// SLOTS    CONTROLLER
//
void LowEnergyAdapter::onControllerConnect()
{
    qDebug() << "[CTRLLER]\tConnected";
    controller->discoverServices();
}
void LowEnergyAdapter::onControllerDisconnect()
{
    qDebug() << "[CTRLLER]\tdisconnected";

    if( controller->error() ==
        (QLowEnergyController::ConnectionError |
        QLowEnergyController::RemoteHostClosedError)
       )
    {
        //hash
        UUID2ParentService.clear();
        //UUID2Charact.clear();

        for(auto &s : Services)
        {
            s->disconnect();
            delete s;
        }
        //list
        Services.clear();
        UUIDServices.clear();

        controller->disconnect();
        delete controller;
        controller = nullptr;
    }
}
void LowEnergyAdapter::onControllerErrorOccur(QLowEnergyController::Error error)
{
    qDebug() << "[CTRLLER]\terror:" << error;
}
void LowEnergyAdapter::onControllerDiscoveryServicesFinish()
{
    qDebug() << "[CTRLLER]\tdiscover finished";

    UUIDServices = controller->services();
    for(auto &u : UUIDServices)
    {
        QLowEnergyService* serv = controller->createServiceObject(u,_parent);
        Services.append(serv);
        connect(
            serv,
            SIGNAL(stateChanged(QLowEnergyService::ServiceState)),
            this,
            SLOT(onServiceStateChange(QLowEnergyService::ServiceState))
        );
        connect(
            serv,
            SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)),
            this,
            SLOT(onServiceCharacteristicChange(QLowEnergyCharacteristic,QByteArray))
        );
        connect(
            serv,
            SIGNAL(characteristicRead(QLowEnergyCharacteristic,QByteArray)),
            this,
            SLOT(onServiceCharacteristicChange(QLowEnergyCharacteristic,QByteArray))
        );
        serv->discoverDetails();


        qDebug() << "[CTRLLER]\t" << u;
    }
}

void LowEnergyAdapter::onServiceStateChange(QLowEnergyService::ServiceState newState)
{
    if(newState == QLowEnergyService::RemoteServiceDiscovered)
        detailFinishCount++;

    if( isAllDetailFinished() )
    {
        qDebug() << "[CTRLLER]\tall detail has been discovered";

        for(auto& s : Services)
        {
            for(auto& c : s->characteristics())
            {
                //UUID2Charact[c.uuid().toString()] = c;
                UUID2ParentService[c.uuid().toString(QUuid::WithoutBraces)] = s;
            }
        }

    }
}
void LowEnergyAdapter::onServiceCharacteristicChange(QLowEnergyCharacteristic c,QByteArray val)
{
    qDebug()<< "[CH]\t"
            << c.uuid().toString()
            << ":" << val;
}
void LowEnergyAdapter::onServiceCharacteristicRead(QLowEnergyCharacteristic c,QByteArray val)
{
    qDebug()<< "[CH_READ]\t"
            << c.uuid().toString()
            << ":" << val;
}
void LowEnergyAdapter::onServiceCharacteristicWritten(QLowEnergyCharacteristic c,QByteArray newVal)
{
    qDebug()<< "[CH_WRITTEN]\t"
            << c.uuid().toString()
            << ":" << newVal;
}


// private method
//
bool LowEnergyAdapter::isAllDetailFinished()
{
    return detailFinishCount == Services.size();
}
