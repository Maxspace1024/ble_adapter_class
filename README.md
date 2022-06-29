# BLE_ADAPTER_CLASS
* 將UI和連線實作分開，便於未來開發和擴充
    ```C++
    //method
    //掃描
    void startScan(int discoveryTimeout);       // milli-second
    void stopScan();
    void showDevices();

    //連線
    void connectToDevice(int index);
    void disconnectFromDevice();

    //對characteristic的操作
    QLowEnergyService* findServiceOfCharacteristic(const QString &);
    QString readCharacteristic(const QString &);
    void writeCharacteristic(const QString &,const QByteArray &);
    ```

## 20220630
* 目前搜尋Bluetooth Device
  * classic & Low Energy 都會捕捉到
  - [ ] 修改為只能抓到 Low Energy 
* 以下Service的SIGNAL沒有捕捉到
    * characteristicRead
    * characteristicWritten
    * characteristicChanged
- [ ] 需增加檢測Characteristic Property的method
- [ ] Notification的功能需要研究
- [ ] 當Service掃描完成(serv->discoverDetail())，`detailFinishCount`的數值會加一
  * 用來判斷所有Service是否抓到自己的Characteristics
  * 這應該不是個好方法，在stateChange中可能還會發生其他狀態
  * 這些"其他狀態"還沒有處理的方法與步驟
  * 如果某個Service細節搜尋持續出錯的話?...
- [ ] 改進以下搜尋方式，線性匹配Characteristic太糟糕了
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
  - [ ] 尚未完成Service詳細掃描前，此功能無效
  - [ ] 有write_respone,read_respone的欄位需要增加