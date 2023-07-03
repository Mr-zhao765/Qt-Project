#include "mylog.h"



//日志重定向 全局函数
void Log(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    QString content;

    //这里可以根据不同的类型加上不同的头部用于区分
    switch (type) {
    case QtDebugMsg:
        content = QString("%1").arg(msg);
        break;

    case QtWarningMsg:
        content = QString("QtWarningMsg: %1").arg(msg);
        break;

    case QtCriticalMsg:
        content = QString("QtCriticalMsg: %1").arg(msg);
        break;

    case QtFatalMsg:
        content = QString("QtFatalMsg: %1").arg(msg);
        break;
    default: break;
    }

    MyLog::Instance()->save(content);
}

/*类外初始化对象*/
QScopedPointer<MyLog> MyLog::self;
MyLog *MyLog::Instance()
{
    if (self.isNull()) {
        if (self.isNull()) {
            self.reset(new MyLog);
        }
    }
    return self.data();
}



MyLog::MyLog(QObject *parent) : QObject(parent)
{

}

MyLog::~MyLog()
{

}


//安装日志钩子,输出调试信息到文件,便于调试
void MyLog::start()
{
    qInstallMessageHandler(Log);
}

//卸载日志钩子
void MyLog::stop()
{
    qInstallMessageHandler(0);
}



void MyLog::save(const QString &content)
{
    if(1)
    {
        QString msg = static_cast<QString>(content);

        if(content.contains("(RTU client) Sent Serial ADU:"))
        {
            msg = msg.remove(msg.indexOf("(RTU client) Sent Serial ADU:"),32);

            msg = msg.left(msg.size());
            msg = msg.toUpper();
            int n = msg.length();
            while(n-2 > 0)
            {
                n = n - 2;
                msg.insert(n," ");
            }
            emit sendModbusData(msg,0);
        }
        else if(content.contains("(RTU client) Received ADU:"))
        {
            msg = msg.remove(msg.indexOf("(RTU client) Received ADU:"),28);
            msg = msg.left(msg.size()-1);
            msg = msg.toUpper();
            int n = msg.length();
            while(n-2 > 0)
            {
                n = n - 2;
                msg.insert(n," ");
            }
            emit sendModbusData(msg,1);
        }
        qDebug()<<"content:"<<content;
    }
}

