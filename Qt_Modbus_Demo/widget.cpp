#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QMessageBox>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    //刷新串口
    on_pushButton_refreshCom_clicked();

    //值类型
    ui->comboBox_valueType->addItem(tr("Coils"), QModbusDataUnit::Coils);
    ui->comboBox_valueType->addItem(tr("Discrete Inputs"), QModbusDataUnit::DiscreteInputs);
    ui->comboBox_valueType->addItem(tr("Input Registers"), QModbusDataUnit::InputRegisters);
    ui->comboBox_valueType->addItem(tr("Holding Registers"), QModbusDataUnit::HoldingRegisters);

    //初始化指针 不然开始指针判断会出错
    modbusDevice = nullptr;

    // 捕获modbus发送缓冲，开启后可以打印下面的信息，然后通过重定向打印功能将将我们需要的数据显示到缓冲区显示
    // qt.modbus: (RTU client) Sent Serial PDU // 发送缓冲区
    // qt.modbus: (RTU client) Received ADU     // 接受缓冲区
    QLoggingCategory::setFilterRules(QStringLiteral("qt.modbus* = true"));
    // qt.modbus: (RTU client) Sent Serial PDU: 0x0300000005
    // qt.modbus.lowlevel: (RTU client) Sent Serial ADU: 0x01030000000585c9
    // qt.modbus: (RTU client) Send successful (quick): 0x0300000005
    // qt.modbus.lowlevel: (RTU client) Response buffer: "01030a000100030008002d00032bef"
    // qt.modbus: (RTU client) Received ADU: "01030a000100030008002d00032bef"

    MyLog::Instance()->start();//通过qInstallMessageHandler 方式捕捉qt.modbus的内容
    connect(MyLog::Instance(),&MyLog::sendModbusData,this,&Widget::modbus_data_display);

}

Widget::~Widget()
{
    delete ui;
}

//获取串口数据
void Widget::getComParameter()
{
    //初始化串口参数信息
    m_settings.serialPort = ui->comboBox_com->currentText();
    m_settings.parity = ui->comboBox_parity->currentIndex();
    if (m_settings.parity > 0)
        m_settings.parity++;
    m_settings.baud = ui->comboBox_baud->currentText().toInt();
    m_settings.dataBits = ui->comboBox_databits->currentText().toInt();
    m_settings.stopBits = ui->comboBox_stopbits->currentText().toInt();
    m_settings.responseTime = ui->spinBox_timeout->value();
    m_settings.numberOfRetries = ui->spinBox_retries->value();
}


//刷新串口
void Widget::on_pushButton_refreshCom_clicked()
{
    QList<QSerialPortInfo> port_list = QSerialPortInfo::availablePorts();

    ui->comboBox_com->clear();
    foreach(const QSerialPortInfo & info,port_list)
    {
//       qDebug() << info.portName(); //串口号 COM1 COM2-----
//       qDebug() << info.systemLocation(); //串口存在的系统位置是个路径
//       qDebug() << info.description();//返回串口描述字符串（如果可用）；否则返回空字符串
//       qDebug() << info.manufacturer();//返回串口制造商字符串（如果可用）；否则返回空字符串
//       qDebug() << info.serialNumber();//返回串口序列号字符串（如果可用）；否则返回空字符串
       ui->comboBox_com->addItem(info.portName());
    }
}


//打开串口
void Widget::on_pushButton_openCom_clicked()
{
    //获取串口数据
    getComParameter();
    if (modbusDevice)
    {
        modbusDevice->disconnectDevice();
        delete modbusDevice;
        modbusDevice = nullptr;
    }

    modbusDevice = new QModbusRtuSerialMaster(this);
    connect(modbusDevice, &QModbusClient::errorOccurred, [this](QModbusDevice::Error) {
        qDebug() << "modbus Error:" << modbusDevice->errorString();
    });

    //配置串口参数
    modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter,m_settings.serialPort);
    modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter,m_settings.parity);
    modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,m_settings.baud);
    modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,m_settings.dataBits);
    modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,m_settings.stopBits);
    modbusDevice->setTimeout(m_settings.responseTime);          // 配置请求超时时间
    modbusDevice->setNumberOfRetries(m_settings.numberOfRetries);     // 配置失败重试次数

    if(!modbusDevice->connectDevice())
    {
        qDebug()<<tr("Connect failed: %1").arg(modbusDevice->errorString());
    }
    else
    {
        qDebug() << "Modbus open Success!";
        ui->pushButton_openCom->setEnabled(false);
        ui->pushButton_closeCom->setEnabled(true);
        ui->pushButton_refreshCom->setEnabled(false);

        ui->comboBox_com->setEnabled(false);
        ui->comboBox_baud->setEnabled(false);
        ui->comboBox_databits->setEnabled(false);
        ui->comboBox_stopbits->setEnabled(false);
        ui->comboBox_parity->setEnabled(false);
        ui->spinBox_timeout->setEnabled(false);
        ui->spinBox_retries->setEnabled(false);
    }
}

//关闭串口
void Widget::on_pushButton_closeCom_clicked()
{
    if (!modbusDevice)
        return;
    modbusDevice->disconnectDevice();
    delete modbusDevice;
    modbusDevice = nullptr;

    qDebug() << "Modbus close Success!";
    ui->pushButton_openCom->setEnabled(true);
    ui->pushButton_closeCom->setEnabled(false);
    ui->pushButton_refreshCom->setEnabled(true);

    ui->comboBox_com->setEnabled(true);
    ui->comboBox_baud->setEnabled(true);
    ui->comboBox_databits->setEnabled(true);
    ui->comboBox_stopbits->setEnabled(true);
    ui->comboBox_parity->setEnabled(true);
    ui->spinBox_timeout->setEnabled(true);
    ui->spinBox_retries->setEnabled(true);
}


//写串口数据
void Widget::on_pushButton_write_clicked()
{
    if (!modbusDevice)
    {
        QMessageBox::information(NULL,  "提示",  "请先连接设备");
        return;
    }

    //获取要写入的寄存器数据
    QList<quint16> values;
    QStringList values_list = ui->lineEdit_writeValue->text().split(" ");
    for(int i = 0 ; i < values_list.size(); i++)
    {
        values.append(values_list.at(i).toUInt());
    }
    int id = ui->lineEdit_id->text().toInt(); //设备地址
    int addr = ui->lineEdit_addr->text().toInt(); //寄存器地址

    //组合写数据帧  table写入的数据类型 寄存器或线圈
    const auto table =
        static_cast<QModbusDataUnit::RegisterType> (ui->comboBox_valueType->currentData().toInt());
    QModbusDataUnit writeUnit = QModbusDataUnit(table,
                                                addr, values.size());
    for(int i=0; i<values.size(); i++)
    {
        writeUnit.setValue(i, values.at(i));
    }

    //id 发生给slave的ID
    if (auto *reply = modbusDevice->sendWriteRequest(writeUnit,id))
    {
        if (!reply->isFinished())
        {
            connect(reply, &QModbusReply::finished, this, [this, reply]()
            {
                if (reply->error() == QModbusDevice::ProtocolError)
                {
                    qDebug() << QString("Write response error: %1 (Mobus exception: 0x%2)")
                                .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16);
                }
                else if (reply->error() != QModbusDevice::NoError)
                {
                    qDebug() << QString("Write response error: %1 (code: 0x%2)").
                                arg(reply->errorString()).arg(reply->error(), -1, 16);
                }
                reply->deleteLater();
            });
        }
        else
        {
            reply->deleteLater();
        }
    }
    else
    {
        qDebug() << QString(("Write error: ") + modbusDevice->errorString());
    }
}


//读串口数据
void Widget::on_pushButton_read_clicked()
{
    if (!modbusDevice)
    {
        QMessageBox::information(NULL,  "提示",  "请先连接设备");
        return;
    }
    //清除读窗口信息
    ui->lineEdit_readValue->clear();

    //获取设备信息
    int id = ui->lineEdit_id->text().toInt(); //设备地址
    int addr = ui->lineEdit_addr->text().toInt(); //寄存器地址
    int readNum = ui->lineEdit_readNum->text().toInt(); //读取寄存器个数

    //组合写数据帧  table写入的数据类型 寄存器或线圈
    const auto table =
        static_cast<QModbusDataUnit::RegisterType> (ui->comboBox_valueType->currentData().toInt());

    QModbusDataUnit readUint = QModbusDataUnit(table,
                                                     addr, readNum);
    //读取数据
    if (auto *reply = modbusDevice->sendReadRequest(readUint, id))
    {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &Widget::readReady);
        else
            delete reply;
    }
    else
    {
        qDebug() << "Read error: " << modbusDevice->errorString();
    }
}


//读取信息
void Widget::readReady()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;

    if (reply->error() == QModbusDevice::NoError)
    {
        const QModbusDataUnit unit = reply->result();
        if(unit.valueCount() == ui->lineEdit_readNum->text().toUInt())
        {
            QString send_buff;
            for (uint i = 0; i < unit.valueCount(); i++)
            {
                const QString entry = tr("Address: %1, Value: %2").arg(unit.startAddress() + i)
                                        .arg(QString::number(unit.value(i),
                                             unit.registerType() <= QModbusDataUnit::Coils ? 10 : 16));
                send_buff.append(QString::number(unit.value(i),
                                                 unit.registerType() <= QModbusDataUnit::Coils ? 10 : 16) + " ");
            }
            //读取的数据
            ui->lineEdit_readValue->insert(send_buff);
        }
    }
    else if (reply->error() == QModbusDevice::ProtocolError)
    {
        qDebug() << QString("Read response error: %1 (Mobus exception: 0x%2)").
                    arg(reply->errorString()).
                    arg(reply->rawResult().exceptionCode(), -1, 16);
    }
    else
    {
        qDebug() << QString("Read response error: %1 (code: 0x%2)").
                    arg(reply->errorString()).
                    arg(reply->error(), -1, 16);
    }
    reply->deleteLater();
}

//获取发送接收数据帧  0发送的数据 1接受的数据
void Widget::modbus_data_display(QString data, int type)
{
    if(type == 0)   //发送
    {
        ui->textBrowser_sendbuf->append(data);
    }
    else if(type == 1)  //接受
    {
        ui->textBrowser_recvbuf->append(data);
    }
}



