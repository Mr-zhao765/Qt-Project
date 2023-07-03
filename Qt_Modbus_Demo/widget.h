#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSerialPortInfo>
#include <QModbusRtuSerialMaster>
#include <QModbusClient>
#include <QSerialPort>
#include "mylog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

//串口参数
struct Settings {
    QString serialPort;
    int parity;
    int baud;
    int dataBits;
    int stopBits;
    int responseTime;
    int numberOfRetries;
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void getComParameter();

private slots:
    void on_pushButton_refreshCom_clicked();

    void on_pushButton_openCom_clicked();

    void on_pushButton_closeCom_clicked();

    void on_pushButton_write_clicked();

    void on_pushButton_read_clicked();

    void readReady();
    void modbus_data_display(QString data, int type);

private:
    Ui::Widget *ui;
    Settings m_settings; //串口参数信息
    QModbusClient *modbusDevice;
};
#endif // WIDGET_H
