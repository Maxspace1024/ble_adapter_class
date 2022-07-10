# BLE_ADAPTER_CLASS
* 將UI和連線實作分開，便於未來開發和擴充
    ```C++
    //method
    //掃描
    void startScan(int discoveryTimeout);       // milli-second
    void stopScan();
    void showDevices();
    bool isScanning();

    //連線
    void connectToDevice(int index);
    void disconnectFromDevice();
    QString getRemoteMacAddress();

    //對characteristic的操作
    QLowEnergyCharacteristic::PropertyTypes getCharacteristicProperties(const QString &);
    QLowEnergyService* findServiceOfCharacteristic(const QString &);
    QLowEnergyCharacteristic findCharacteristicObject(const QString &);
    QByteArray readCharacteristic(const QString &);
    void writeCharacteristic(const QString &,const QByteArray &,int);
    void enableCharacteristicNotification(const QString &,bool);
    void enableCharacteristicIndication(const QString &,bool);

    //檢測是否掃描完成所有Service中的Characteristic
    bool LowEnergyAdapter::isAllDetailFinished()
    ```

## 20220710
* LowEnergyAdapter新建method
  - [X] mac device
  - [X] agent status
  - [X] connectToDevice 要防呆
  - [X] `onServiceCharacteristicChange()` modify the parameter of function signature
* 增加進度條

## 20220708
* 在`LowEnergyAdapter`中的signal`adapterCharacteristicChange(QLowEnergyService*,const QLowEnergyCharacteristic &)`
  * 此寫法破壞Encapsulation，內部資料外洩
  * Service & Characteristic 應該設為private，外部必須無法讀取
  - [ ] 或許改成回傳(UUID_STRING,QByteArray)
* 增加signal告知掃描Service內部完成了
* `writeCharacteristic()`增加count參數(parameter)，用來truncat QByteArray
  * 有限數量的寫入
* `readCharacteristic()`修改回傳型態
  * ~~QString~~
  * QByteArray
* 阻止掃描時期的錯誤操作
  * read
  * write
  * enableNotification
  * enableIndication

## 20220630晚上
* 抓到戰犯，記得匯入標頭
  ```C
  #include <QLowEnergyService>
  #include <QLowEnergyCharacteristic>

  QList<QLowEnergyService*> Services;
  QHash< QString , QLowEnergyService* > UUID2ParentService;
  ```
* 新增啟用Indication & Notification的方法
  * 需使用靜態函數`QLowEnergyCharacteristic::CCCDEnable*`
  * 可以固定收到Sensor回傳的數值
  - [ ] 了解該寫入什麼數值至特定Characteristic，並閱讀Service 說明文件
* 新增getCharacteristicProperties方法
* 新增程式註解說明
- [ ] characteristicChange()也需要emit SIGNAL, for the signal of `LowEnergyAdapter`
  * 外部使用adapter的時候可以接收其SIGNAL
  * 增加SIGNAL: `adapterCharacteristicChange(_,?)`

## 20220630凌晨
* 目前搜尋Bluetooth Device
  * classic & Low Energy 都會捕捉到
  - [X] 修改為只能抓到 Low Energy 
* 以下Service的SIGNAL沒有捕捉到
    * characteristicRead
    * characteristicWritten
    * ~~characteristicChanged~~
- [X] 需增加檢測Characteristic Property的method
- [X] Notification的功能需要研究
- [X] 當Service掃描完成(serv->discoverDetail())，`detailFinishCount`的數值會加一
  * 用來判斷所有Service是否抓到自己的Characteristics
  * 這應該不是個好方法，在stateChange中可能還會發生其他狀態
  * 這些"其他狀態"還沒有處理的方法與步驟
  * 如果某個Service細節搜尋持續出錯的話?...
- [X] 改進以下搜尋方式，線性匹配Characteristic太糟糕了
  * 快速的在Service中，找到目標Characteristic
  * 前面嘗試`QHash<QString uuid, QLowEnergyCharacteristic c> hash`，導致heap空間有問題所以放棄
    ```C++
    //readCharacteristic & WriteCharacteristic
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
    ```
* WRITE_btn 需寫入長度相同的ByteArray & READ_btn 的讀取限制
  - [X] 尚未完成Service詳細掃描前，此功能無效
  - [X] 有write_respone的欄位需要增加
* Hash member 會出現heap動態記憶體位置問題
  ```
  HEAP[bledevclass.exe]: Heap block at 000002857A9DC500 modified at 000002857A9DC5CC past requested size of bc
  ```

* * *
* * * 
## QBluetoothDeviceDiscoveryAgent
* 最基礎的函數，用來尋找周圍的藍牙裝置

<br/>

### *__METHODS__*
* **setLowEnergyDiscoveryTimeout( int timeout )**
    * 設定LE搜尋時間長度
* **start()**
    * 開始搜尋周圍裝置
    * 設定搜尋裝置型態
**stop()**
    * 終止搜尋裝置

### *__SIGNAL__*
* **canceled()**
    * 當使用`stop()會發出此信號
* **finished()**
    * 當timeout的時候會發出此信號
* **deviceDiscovered(const QBluetoothDevice &device)**
    * 偵測到裝置發出此信號
```c
//agent is a pointer
agent = new QBluetoothDeviceDiscoveryAgent(this);
agent->setLowEnergyTimeout(10000);
agent->start()
```

* * *
## QBluetoothDeviceInfo
* 儲存裝置資訊
* * *
## QBluetoothLocalDevice
* 儲存本地裝置資訊
* * *
## QLowEnergyController
* 用來建立LowEnergy連線

### *__METHODS__*
* **QLowEnergyController::createCentral( *REMOTE_DEVICE*, *LOCAL_ADDRESS*, *PARENT*)**
    * return a pointer
* **connectToDevice()**
    * 連線至remote device
* * *
## QLowEnergyService
* 需使用 `QLowEnergyController::createServiceObject()` 建立service
* `discoverDetails()`掃描所有Characteristic
  