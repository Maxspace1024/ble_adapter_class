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
            SLOT(onDeviceDiscover(QBluetoothDeviceInfo))
    );
    connect(agent,
            SIGNAL(errorOccurred(QBluetoothDeviceDiscoveryAgent::Error)),
            this,
            SLOT(onAgentErrorOccur(QBluetoothDeviceDiscoveryAgent::Error))
    );

    localDevice = new QBluetoothLocalDevice(this);
}
LowEnergyAdapter::~LowEnergyAdapter()
{
    if(controller!=nullptr)
    {
        delete controller;
        controller=nullptr;
    }
    delete agent;
    delete localDevice;
}

// PUBLIC

void LowEnergyAdapter::startScan(int discoveryTimeout)
{
    peripheralDevices.clear();
    agent->setLowEnergyDiscoveryTimeout(discoveryTimeout);

    //prevent double scan
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
    //the central device can't show peripheral devices when it is scanning
    if(!agent->isActive())
    {
        int i=0;
        qDebug() << "=======devices=======";
        for(auto &d : peripheralDevices)
            qDebug() << i++ << " " << d.name();
        qDebug() << "\n";
    }
    else
        qDebug() << "[AGENT]\tagent is active";
}
bool LowEnergyAdapter::isScanning()
{
    return agent->isActive();
}

void LowEnergyAdapter::connectToDevice(int index)
{
    if(agent->isActive())
    {
        qDebug() << "[AGENT]\tagent is active";
        return;
    }

    //controller should be nullptr, means it is available to initialize
    if(0<= index && index < peripheralDevices.size() && controller == nullptr)
    {
        //init the value of calculating the finished detail Scanning Service
        detailFinishCount=0;

        //select peripheral device, and provide local device address info
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
        connect(controller,
                SIGNAL(stateChanged(QLowEnergyController::ControllerState)),
                this,
                SLOT(onControllerStateChanged(QLowEnergyController::ControllerState))
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
        UUID2Charact.clear();

        for(auto &s : Services)
        {
            // 'cause service is a pointer
            // it should be free when disconnect from peripheral device
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
QString LowEnergyAdapter::getRemoteMacAddress()
{
    if(controller==nullptr)
    {
        qDebug() << "[getMAC]\t ctrller is invalid";
        //return "N/A";
        return QBluetoothAddress().toString();
    }
    else
        return controller->remoteAddress().toString();
}

QLowEnergyCharacteristic::PropertyTypes LowEnergyAdapter::getCharacteristicProperties(const QString &uuid)
{
    //QLowEnergyService* serv = findServiceOfCharacteristic(uuid);
    QLowEnergyCharacteristic c = findCharacteristicObject(uuid);

    if(c.uuid().toString(QUuid::WithoutBraces) != "00000000-0000-0000-0000-000000000000")
        return c.properties();
    else
    {
        qDebug() << "[Property]\tUnknown";
        return QLowEnergyCharacteristic::Unknown;
    }
}
QLowEnergyService* LowEnergyAdapter::findServiceOfCharacteristic(const QString &uuid)
{
    if(UUID2ParentService.contains(uuid))
        return UUID2ParentService[uuid];
    else
        return nullptr;
}
QLowEnergyCharacteristic LowEnergyAdapter::findCharacteristicObject(const QString &uuid)
{
    if(UUID2Charact.contains(uuid))
        return UUID2Charact[uuid];
    else
        return QLowEnergyCharacteristic();
}
QByteArray LowEnergyAdapter::readCharacteristic(const QString &uuid)
{
    QByteArray err;
    err = 0;

    if(!isAllDetailFinished())
    {
        // ??????
        qDebug() << "[READch]\t detail discovery does not finish";
        return err;
    }

    QLowEnergyService* serv = findServiceOfCharacteristic(uuid);
    QLowEnergyCharacteristic c = findCharacteristicObject(uuid);

    if(c.uuid().toString(QUuid::WithoutBraces) != "00000000-0000-0000-0000-000000000000")
    {
        if( c.properties() & QLowEnergyCharacteristic::Read)
        {
            serv->readCharacteristic(c);
            return c.value();
        }
        else
        {
            qDebug() << "[READch]\tProperty Prohibited";
            return err;
        }
    }
    else
    {
        qDebug() << "[READch]\tcharacteristic not found";
        return err;
    }
}
void LowEnergyAdapter::writeCharacteristic(const QString &uuid,const QByteArray &ba,int count)
{
    if(!isAllDetailFinished())
    {
        // ??????
        qDebug() << "[WRITEch]\t detail discovery does not finish";
        return;
    }

    if(ba.size()<count || count<0)
        count = ba.size();

    QLowEnergyService* serv = findServiceOfCharacteristic(uuid);
    QLowEnergyCharacteristic c = findCharacteristicObject(uuid);

    if(c.uuid().toString(QUuid::WithoutBraces) != "00000000-0000-0000-0000-000000000000")
    {
        //writing if it holds the permission
        if(c.properties() & QLowEnergyCharacteristic::Write)
            serv->writeCharacteristic(c,ba.first(count));
        else
            qDebug() << "[WRITEch]\tProperty Prohibited";
        
        /*
        if( ba.size() <= c.value().size())
        {
            

        }
        else
            qDebug() << "[WRITEch]\tarray size does not match";
        */
    }
    else
        qDebug() << "[WRITEch]\tcharacteristic not found";
}
void LowEnergyAdapter::enableCharacteristicNotification(const QString &uuid,bool flag)
{
    if(!isAllDetailFinished())
    {
        // ??????
        qDebug() << "[enableNotif]\t detail discovery does not finish";
        return;
    }

    QLowEnergyService* serv = findServiceOfCharacteristic(uuid);
    QLowEnergyCharacteristic c = findCharacteristicObject(uuid);

    if(c.uuid().toString(QUuid::WithoutBraces) != "00000000-0000-0000-0000-000000000000")
    {
        auto cccd = c.clientCharacteristicConfiguration();
        if(cccd.isValid())
        {
            if(flag)                    //flag=true : enable
                serv->writeDescriptor(cccd,QLowEnergyCharacteristic::CCCDEnableNotification);
            else
                serv->writeDescriptor(cccd,QLowEnergyCharacteristic::CCCDDisable);
        }
        else
            qDebug() << "[SET_NOTIFY]\t fail";
    }
}
void LowEnergyAdapter::enableCharacteristicIndication(const QString &uuid,bool flag)
{
    if(!isAllDetailFinished())
    {
        // ??????
        qDebug() << "[enableIndicat]\t detail discovery does not finish";
        return;
    }

    QLowEnergyService* serv = findServiceOfCharacteristic(uuid);
    QLowEnergyCharacteristic c = findCharacteristicObject(uuid);

    if(c.uuid().toString(QUuid::WithoutBraces) != "00000000-0000-0000-0000-000000000000")
    {
        auto cccd = c.clientCharacteristicConfiguration();
        if(cccd.isValid())
        {
            if(flag)                    //flag=true : enable
                serv->writeDescriptor(cccd,QLowEnergyCharacteristic::CCCDEnableIndication);
            else
                serv->writeDescriptor(cccd,QLowEnergyCharacteristic::CCCDDisable);
        }
        else
            qDebug() << "[SET_INDICAT]\t fail";
    }
}
int LowEnergyAdapter::getServiceSize()
{
    return Services.size();
}
bool LowEnergyAdapter::isAllDetailFinished()
{
    return detailFinishCount == Services.size();
}


// SLOTS    AGENT
//
void LowEnergyAdapter::onScanFinish()
{
    qDebug() << "[AGENT]\tscan finished";
}
void LowEnergyAdapter::onScanCancel()
{
    qDebug() << "[AGENT]\tscan canceled";
}
void LowEnergyAdapter::onDeviceDiscover(const QBluetoothDeviceInfo &info)
{
    //appen to list when device type is Low Energy
    if( info.coreConfigurations() == QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
    {
        qDebug() << "[AGENT]\tcatch:"
                 << info.name();
        peripheralDevices.append(info);
        emit adapterDeviceDiscovered(info);
    }
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
    controller->discoverServices();             //discover the service in the peripheral device
}
void LowEnergyAdapter::onControllerDisconnect()
{
    qDebug() << "[CTRLLER]\tdisconnected";

    // clear some member for normally using in the next connection

    if( controller->error() ==
        (QLowEnergyController::ConnectionError |
        QLowEnergyController::RemoteHostClosedError)
       )
    {
        //hash
        UUID2ParentService.clear();
        UUID2Charact.clear();

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
            SLOT(onServiceCharacteristicRead(QLowEnergyCharacteristic,QByteArray))
        );
        connect(
            serv,
            SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)),
            this,
            SLOT(onServiceCharacteristicWritten(QLowEnergyCharacteristic,QByteArray))
        );
        serv->discoverDetails();


        qDebug() << "[CTRLLER]\t" << u;
    }
}
void LowEnergyAdapter::onControllerStateChanged(QLowEnergyController::ControllerState state)
{
    qDebug() << "[CTRLLER_STAT]\t" << state;
    emit adapterConnectionStateChanged(state);
}

// SLOTS    SERVICE
//
void LowEnergyAdapter::onServiceStateChange(QLowEnergyService::ServiceState newState)
{
    if(newState == QLowEnergyService::RemoteServiceDiscovered)
    {
        detailFinishCount++;
        emit adapterServiceDetailDiscovered(detailFinishCount);
    }

    if( isAllDetailFinished() && newState != QLowEnergyService::InvalidService)
    {
        qDebug() << "[CTRLLER]\tall detail has been discovered";

        // make a mapping from UUID_STRING -> INSTANCE
        for(auto& s : Services)
        {
            for(auto& c : s->characteristics())
            {
                //insert the mapping form [uuid string] to [service pointer] into hash
                UUID2ParentService[c.uuid().toString(QUuid::WithoutBraces)] = s;
                UUID2Charact[c.uuid().toString(QUuid::WithoutBraces)] = c;
            }
        }

        // send signal that means discovery is done
        emit adapterDiscoverDetailFinished(detailFinishCount);
    }
}
void LowEnergyAdapter::onServiceCharacteristicChange(QLowEnergyCharacteristic c,QByteArray val)
{
    qDebug()<< "[CH_CHANG] "
            << c.uuid().toString()
            << ":" << val.toHex(' ');

    // ???????????????Service & Characteristic??????private
    // ???????????????????????????
    //QLowEnergyService* serv = findServiceOfCharacteristic(c.uuid().toString(QUuid::WithoutBraces));
    //emit adapterCharacteristicChange(serv,c);

    emit adapterCharacteristicChanged(
        c.uuid().toString(QUuid::WithoutBraces),
        c.value()
    );
}
void LowEnergyAdapter::onServiceCharacteristicRead(QLowEnergyCharacteristic c,QByteArray val)
{
    qDebug()<< "[CH__READ] "
            << c.uuid().toString()
            << ":" << val;
}
void LowEnergyAdapter::onServiceCharacteristicWritten(QLowEnergyCharacteristic c,QByteArray newVal)
{
    qDebug()<< "[CH_WRITEN]"
            << c.uuid().toString()
            << ":" << newVal;
}
