#ifndef MYLOG_H
#define MYLOG_H

#include <QObject>
#include <QLoggingCategory>

class MyLog : public QObject
{
    Q_OBJECT
public:
    explicit MyLog(QObject *parent = nullptr);
    ~MyLog();
    static MyLog *Instance();



public slots:
    //启动日志服务
    void start();
    //暂停日志服务
    void stop();
    //保存日志
    void save(const QString &content);

signals:
    void sendModbusData(QString,int);


private:
    static QScopedPointer<MyLog> self;


};

#endif // MYLOG_H
