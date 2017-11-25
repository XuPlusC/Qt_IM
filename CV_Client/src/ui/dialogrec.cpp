#include "dialogrec.h"
#include "ui_dialogrec.h"


DialogRec::DialogRec(QWidget *parent):
    QDialog(parent),
    ui(new Ui::DialogRec)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("文件传输！"));
    port = "0";

#ifndef TCP
    udpsocket = new QUdpSocket();
#else
    tcpSocket = new QTcpSocket();
    connect(tcpSocket,&QTcpSocket::readyRead,
            [=](){
        QByteArray buf = tcpSocket->readAll();
        if(true == isStart){
            isStart = false;
            //接收包头
            fileName = QString(buf).section("##",0,0);
            fileSize = QString(buf).section("##",1,1).toInt();
            recvSize = 0;

            QString str = QString("接收的文件:[%1:%2kB]").arg(fileName).arg(fileSize/1024);
            setWindowTitle(str);

            file.setFileName(fileName);
            if(false == file.open(QIODevice::WriteOnly)){
                QMessageBox::information(this,"Error","文件创建并打开失败!");
            }
            tcpSocket->write("FileHead recv");
        }else{
            //接收处理文件
            qint64 len = file.write(buf);
            recvSize += len;
            if(recvSize == fileSize){//接收完毕
                file.close();
                //提示信息
                QMessageBox::information(this,"完成","文件接收完成");
                //回射信息
                tcpSocket->write("file write done");
                tcpSocket->disconnectFromHost();
                tcpSocket->close();
            }
        }
    }
    );
#endif
}

void DialogRec::setFileReq(JSPP fileReq)
{
    fileName = QString::fromStdString(fileReq.body).split("&")[0];;
    peer_user = fileReq.from;
    ui->label->setText(QString::fromStdString(peer_user) +
                       "向您发送文件" +
                       fileName.split("/").back() +
                       "是否接收？");
    port = QString::fromStdString(fileReq.body).split("&")[1].toStdString();
}


DialogRec::~DialogRec()
{
    delete ui;
}



void DialogRec::on_okBtn_clicked()
{
#ifndef TCP
    udpsocket->bind(QHostAddress::Any, 10086);
    connect(udpsocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
    port = "10086";
#else
    tcpSocket->connectToHost(QString("127.0.0.1"),QString::fromStdString(port).toInt());
#endif
    //todo 动态获取端口
    QString body = fileName + QString::fromStdString("*#*") + QString::fromStdString(port);
    JSPP msg;
    msg.type = "file";
    msg.body = body.toStdString();
    msg.from = IMClient::Instance().getCurrID();
    msg.to = peer_user;
    msg.code = "1";
    IMClient::Instance().sendMsg(msg);
    isStart = true;
}

void DialogRec::on_noBtn_clicked()
{
    QString body = "";
    JSPP msg;
    msg.type = "file";
    msg.body = body.toStdString();
    msg.from = IMClient::Instance().getCurrID();
    msg.code = "2";
    msg.to = peer_user;
    IMClient::Instance().sendMsg(msg);
    hide();
}
void DialogRec::readPendingDatagrams()
{
    QFile file(fileName.split("/").back());
    if(!file.open(QIODevice::ReadWrite)) return;
    file.resize(0);//清空原有内容
#ifndef TCP
    while(udpsocket->hasPendingDatagrams()){
        QByteArray datagram;
        datagram.resize(udpsocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 Pic_port;
        udpsocket->readDatagram(datagram.data(),datagram.size(),&sender,&Pic_port);
        if(datagram!="End!") {
            file.write(datagram.data(),datagram.size());
            qDebug()<<datagram.size()<<endl;
        }else{
            break;
        }
    }
#else

#endif
    file.close();
    //udpsocket->close();
    QMessageBox::warning(this,tr("通知"),tr("接收完成"),QMessageBox::Yes);
    hide();
}
