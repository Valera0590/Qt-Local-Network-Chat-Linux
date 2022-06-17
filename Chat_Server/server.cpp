#include "server.h"


Server::Server()
{
    udpSocket = new QUdpSocket(this);   //coздаëм обьект соkета QUdpSocket

    udpSocket->bind(QHostAddress::Broadcast, 2222); //задаём широковещательный адрес и порт на который сокет будет получать данные
    connect (udpSocket, SIGNAL (readyRead()), this, SLOT (UDPReadingData()));  //для получения и отображения данныx соединяем сигнал сокета со слотом
    qDebug() << "Server start listening udp socket with port 2222";
    filePath.clear();
    fileSize = 0;
    tmpBlock.clear();
    fileCopy = QDir::homePath() + "/download"; // временная замена имени файла
}

void Server::UDPReadingData()
{
    while(udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());  //узнаем размер ждущей обработки "датаграммы"
        udpSocket->readDatagram(datagram.data(),datagram.size(), &sender, &senderPort); //читаем данные
        qDebug()<<"";
        qDebug()<<datagram.data()<<"IP: " + sender.toString()<<"Port: "+QString("%1").arg(senderPort);
        login = datagram.data();
    }
    QString str = sender.toString();
    qDebug()<<"";
    qDebug() <<"Client"<<login<<"connected with server by udp.\nSend to client"<<login<<"server's ip on new port: " << quint16(senderPort)+1;
    QThread::msleep(200);   //пауза для завершения установки порта прослушивания в клиентском приложении
    if(!isClientConnected)  //после подключения первого клиента запускаем слушатель TcpSocket
    {
        startTcpServerListening();
        isClientConnected = true;
    }
    udpSocket->writeDatagram(str.toUtf8(), QHostAddress::Broadcast, int(senderPort)+1);  //отправляем данные по широковешательному адресу на порт 2222
}

void Server::startTcpServerListening()
{
    if(this->listen(QHostAddress::Any, 2225))  //задаем TcpSocket прослушивать сообщения с любого адреса по порту 2225
    {
        qDebug() << "\nServer start listening tcp socket with port: 2225";
    }
    else
    {
        qDebug() << "\nError listening tcp socket with port: 2225";
    }
}

void Server::incomingConnection(qintptr socketDescriptor){
    tcpSocket = new QTcpSocket(this);
    tcpSocket->setSocketDescriptor(socketDescriptor);
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));   //связь события получения сообщения с обработчиком
    Sockets.push_back(tcpSocket);
    qDebug()<<"";
    qDebug()<<"Client"<<login<<"connected by tcp with socket descriptor: " << socketDescriptor;
    sendToClients("You were connecting");
    qDebug() << "Send client"<<login<<"message about successfully connection";
}

void Server::slotReadyRead()
{
    tcpSocket = (QTcpSocket*) QObject::sender();    //получение сокета, по которому пришло сообщение
    QDataStream in(tcpSocket);
    in.setVersion(QDataStream::Qt_5_12);
    if(in.status() == QDataStream::Ok)
    {

        if (!isDownloading) //если файл ещё не скачивается
        {
            qDebug()<<"";
            qDebug() << "read...........";
            QString str;
            in.startTransaction();
            in >> str;
            if (!in.commitTransaction()) {
                qDebug() << "SERVER: Str - FAIL commitTransaction";
                return;
            }
            QStringList strList = str.split(";");
            if(strList.value(0) == "msg")
            {
                qDebug() <<str;
                qDebug() << "Message by "+strList.value(1)+": "+strList.value(2);
                sendToClients(strList.value(1)+";"+strList.value(2));
            }
            else if(strList.value(0) == "get file") //отправка файла клиенту, который запросил файл
            {
                login = strList.value(1);
                flNm = strList.value(2);
                filePath = fileCopy + "/" + flNm;
                if(filePath != "")
                {
                    QFile file(filePath);
                    QFileInfo fileInfo(file);
                    fileSize = fileInfo.size();

                    QString infoAboutFileOnServer = "Send file - "+fileInfo.fileName()+" ("+QString::number((long)fileSize)+" bytes) to client "+login;
                    qDebug()<<"";
                    qDebug() << infoAboutFileOnServer;
                    sendFullFileToClient();
                }
                else
                {
                    qDebug() << "\nPath to file is incorrect!";
                }

            }
            else if(strList.value(0) == "download file") //загрузка данных о файле на сервер от клиента
            {
                //====================================================
                    // Получение filePath и fileSize
                    isDownloading = true;
                    fileSize = 0;
                    login = strList.value(1);
                    flNm = strList.value(2);
                    filePath = fileCopy + "/" + flNm;
                    fileSize = strList.value(3).toLongLong();
                    if (QFile::exists(filePath))
                        if (!QFile::remove(filePath))
                            qFatal("File not remove!");
                    qDebug() << "Server was starting get file - "+strList.value(2)+" from client "+strList.value(1);
            }
        }
        else
        {
            //====================================================
            // Получение файла
            in.startTransaction();
            in >> tmpBlock;
            if (!in.commitTransaction())    //ждём полной передачи файла через сокет
                return;

            QFile file(filePath);
            if(file.open(QIODevice::WriteOnly))
            {
                file.write(tmpBlock);
                qDebug() << "\nData has been written to a file!";
            }
            else
            {
                qDebug() << "\nThe file don't open to write a data!";
                return;
            }
            file.close();
            qDebug() << "SERVER: END - bytesAvailable:" << tcpSocket->bytesAvailable();

            // Очистка переменных
            tmpBlock.clear();
            isDownloading = false;
            qDebug()<<"";
            qDebug() << "Send to clients info about file "+flNm;
            sendToClients("fileInfo;"+login+";"+flNm+";"+QString::number((long)fileSize));
        }
    }
    else
    {
        qDebug() << "\nDataStream error";
    }

}

void Server::sendToClients(QString str)
{
    Data.clear();
    QDataStream out(&Data,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << str;

    for (int i = 0; i < Sockets.size(); ++i)
    {
        if (str == "You were connecting")
        {
            if (Sockets[i] == tcpSocket)
            {
                Sockets[i]->write(Data);
                return;
            }
        }
        else
            Sockets[i]->write(Data);
    }

}

void Server::sendFullFileToClient()
{
    //QThread::msleep(100);
    QDataStream stream(tcpSocket);
    stream.setVersion(QDataStream::Qt_5_12);

    QFile* sendFile = new QFile(filePath);
    if(sendFile->open(QFile::ReadOnly))
    {
        QByteArray dataS = sendFile->readAll();
        stream << dataS;
        tcpSocket->waitForBytesWritten();
        qDebug() << "_SERVER: File end!";
        sendFile->close();
        sendFile = NULL;
        qDebug() << "_SERVER: File send FINISH!";
    } else {
        qFatal("_SERVER: File not open!");
    }

}
