#ifndef SERVER_H
#define SERVER_H
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>
#include <QDataStream>
#include <QtNetwork>
#include <QAbstractSocket>
#include <QFileInfo>

class Server: public QTcpServer{
    Q_OBJECT

public:
    Server();
    QTcpSocket *tcpSocket;
    bool isClientConnected = false;
    QHostAddress sender;    //agpec
    quint16 senderPort; //nopт отправителя

//public slots:
    //void ReadingData(); //чтение данных

private:
    QUdpSocket *udpSocket;  //указатель на объект класса QUdpSocket
    QVector <QTcpSocket*> Sockets;    //вектор подключенных сокетов
    QByteArray Data;
    QString login = "";
    QString flNm = "";
    QString fileCopy; // Путь файла для сохранение
    QString filePath;
    qint64 fileSize;
    QByteArray tmpBlock;
    bool isDownloading = false;

    void sendToClients(QString str);
    void sendFullFileToClient();

public slots:
    void incomingConnection(qintptr socketDescriptor);  //слот для подключения клиента
    void startTcpServerListening(); //запуск слушателя сервера TcpSocket
    void slotReadyRead();   //обработчик полученных сообщений от клиента
    //void slotSockDisc();    //обработчик отключения сотека
    void UDPReadingData();  //обработчик сообщений от клиента
};



#endif // SERVER_H
