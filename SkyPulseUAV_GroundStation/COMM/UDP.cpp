#include "UDP.h"
#include <QNetworkDatagram>

UDP::UDP(QObject *parent) : QObject(parent), udpSocket(new QUdpSocket(this))
{
    UDPThread = new QThread(this);
    connect(UDPThread, &QThread::started, this, &UDP::udpInitial);
    connect(UDPThread, &QThread::finished, this, &QObject::deleteLater);
    this->moveToThread(UDPThread);
    UDPThread->start();
}

UDP::~UDP()
{
    if (UDPThread->isRunning()){
        UDPThread->wait();
        UDPThread->quit();
    }
}

void UDP::udpInitial()
{
    connect(udpSocket, &QUdpSocket::readyRead, this, &UDP::readPendingDatagrams);
}

void UDP::startUDPServer(quint16 port)
{
    bool result = udpSocket->bind(port, QUdpSocket::ShareAddress);
    if(!result){
        // 绑定失败，可以处理错误，比如记录日志或者通知用户
        qDebug() << "Unable to bind UDP socket on port" << port << ":" << udpSocket->errorString();
        return;
    }
    emit ServerStartSucessful();
}

void UDP::stopUDPServer()
{
    udpSocket->close();
    if (udpSocket->state() == QAbstractSocket::UnconnectedState) {
        qDebug() << "UDP socket closed successfully.";
        emit ServerStopSucessful();
    } else {
        // 这种情况不太可能发生，但你可以根据需要处理它
        qDebug() << "UDP socket might not have closed properly.";
    }
}

void UDP::readPendingDatagrams()
{
    qDebug() << "UDP Current Thread ID: " << QThread::currentThreadId();
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QString message = QString::fromUtf8(datagram.data());
        emit messageReceived(message);
    }
}
