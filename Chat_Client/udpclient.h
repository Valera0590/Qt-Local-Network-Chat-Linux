#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFileDialog>
#include <QFileInfo>
#include <QThread>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui { class ChatClient; }
QT_END_NAMESPACE

class ChatClient : public QMainWindow
{
    Q_OBJECT

public:
    ChatClient(QWidget *parent = nullptr);
    ~ChatClient();
    QUdpSocket *udpSocket;  //указатель на объект класса QUdpSocket
    QTcpSocket *tcpSocket;  //указатель на объект класса QTcpSocket
    QByteArray datagram;
    QByteArray dataTcp;

signals:
    void disconnected(void);


public slots:
    void slotReadingUDPData(); //чтение данных с udpSocket
    void slotReadingTcpData(); //чтение данных с tcpSocket
    void slotSockDisc();    //отключение сокета

private slots:
    void on_ButtonSend_clicked();
    void on_ButtonConnect_clicked();
    void sendToServer(QString str);
    void sendToServer(QByteArray dataFile);

    void on_lineEditMessage_returnPressed();
    void displayError(QAbstractSocket::SocketError socketError);
    //qint64 percentage(qint64 max, qint64 work);
    void sendFullFile();


    void on_downloadFile_clicked();

    void on_sendFile_clicked();

private:
    Ui::ChatClient *ui;
    QHostAddress ipServer;
    quint16 portThisClient = 1;
    quint16 portTcpServer = 2225;
    QString message = "";
    QString filePath = "";
    QString filePathSave = "";
    QString fileName = "";
    qint64 fileSize;
    qint64 sizeSendData;
    QFile *sendFile;
    bool isDownloading = false;
    qint8 countInterruptGettingFile = 0;
    QByteArray fileBlocks;
    void socketSendMessageFile();
};
#endif // UDPCLIENT_H
