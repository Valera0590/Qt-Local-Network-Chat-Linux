#include "udpclient.h"
#include "ui_udpclient.h"

ChatClient::ChatClient(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ChatClient)
{
    ui->setupUi(this);
    ui->ButtonSend->setEnabled(false);
    ui->lineEditMessage->setEnabled(false);
    ui->downloadFile->setEnabled(false);
    ui->sendFile->setEnabled(false);
    udpSocket = new QUdpSocket(this);   //coздаëм обьект соkета QUdpSocket
    connect (udpSocket, SIGNAL (readyRead()), this, SLOT (slotReadingUDPData()));  //для получения и отображения данныx соединяем сигнал сокета со слотом
    fileBlocks.clear();
}

ChatClient::~ChatClient()
{
    delete udpSocket;
    delete tcpSocket;
    delete ui;
}

void ChatClient::on_ButtonConnect_clicked()
{
    qDebug() << "Try to connect with server by UDP.";
    udpSocket->writeDatagram(ui->lineEditLogin->text().toUtf8(), QHostAddress::Broadcast, 2222);  //отправляем данные по широковешательному адресу на порт 2222
    portThisClient += udpSocket->localPort();
    udpSocket->abort();
    udpSocket->bind(QHostAddress::Broadcast, portThisClient); //задаём широковещательный адрес и порт на который сокет будет получать данные


    tcpSocket = new QTcpSocket(this);
    connect (tcpSocket, SIGNAL (readyRead()), this, SLOT (slotReadingTcpData()));  //для получения и отображения данныx соединяем сигнал сокета со слотом
    connect (tcpSocket, SIGNAL (disconnected()), this, SLOT (slotSockDisc()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
}


void ChatClient:: slotReadingUDPData()  //чтение данных
{
    QHostAddress sender;    //agpec
    quint16 senderPort; //nopт отправителя
    while(udpSocket->hasPendingDatagrams())
    {
        datagram.resize(udpSocket->pendingDatagramSize());  //узнаем размер ждущей обработки "датаграммы"
        udpSocket->readDatagram(datagram.data(),datagram.size(), &sender, &senderPort); //читаем данные
        //ui->textEditMessages->append(QString(datagram) + " IP: " + sender.toString() + " Port: " + QString("%1").arg(senderPort));
        qDebug()<<datagram.data()<<"IP: " + sender.toString()<<"Port: "+QString("%1").arg(senderPort);

    }
//    udpSocket->disconnect();
//    udpSocket->close();
//    udpSocket->abort();

    ipServer = sender;//QHostAddress(datagram.data());
    qDebug()<<"IP: "<<ipServer.toString() + " Port: "+QString("%1").arg(portTcpServer);
    tcpSocket->connectToHost(ipServer, portTcpServer);   //соединяемся с сервером по протоколу Tcp
    const int Timeout = 5 * 1000;
    if (!tcpSocket->waitForConnected(Timeout))
    {
        emit displayError(tcpSocket->error());
        return;
    }
    ui->lineEditLogin->setReadOnly(true);
    ui->lineEditLogin->setEnabled(false);
    ui->ButtonConnect->setEnabled(false);
    ui->ButtonSend->setEnabled(true);
    ui->lineEditMessage->setEnabled(true);
    udpSocket->close();
}

void ChatClient::slotReadingTcpData()
{
    qDebug() << "reading tcpSocket.";
    QDataStream in(tcpSocket);
    in.setVersion(QDataStream::Qt_5_12);
    if(in.status() == QDataStream::Ok)
    {
        if(!isDownloading)
        {
            QString str;
            in >> str;
            QStringList strList = str.split(";");
            if(str == "You were connecting")  //если получено сообщение о подключении клиента
            {
                ui->sendFile->setEnabled(true);
                ui->textEditMessages->append("<font color=blue><B>Вы подключились к чату!</B></font>" + ui->lineEditMessage->text());
            }
            else if(strList.value(0) != "fileInfo" && strList.value(0) != "file")   //обработчик сообщения
            {
                qDebug() << str << " 0:" << strList.value(0) << " 1:" << strList.value(1);
                if(ui->lineEditLogin->text() == strList.value(0))
                    ui->textEditMessages->append("<font color=blue><B>Вы: </B></font>" + strList.value(1));
                else ui->textEditMessages->append("<B>"+strList.value(0)+"</B>: "+strList.value(1));
            }
            else if(strList.value(0) == "fileInfo") //обработчик загруженного на сервер файла
            {
                qDebug() << "Get info about downloading file...";
                qDebug() << str;
                fileName = strList.value(2);
                fileSize = strList.value(3).toLongLong();
                if (fileSize < 1024)    //байты
                {
                    double flsz = fileSize;
                    if(ui->lineEditLogin->text() == strList.value(1))
                        ui->textEditMessages->append("<br>Вы отправили файл - <i>"+fileName+"</i><br>  Размера <i>"+QString::number((double)flsz)+" байт</i>");
                    else    ui->textEditMessages->append("<br>"+strList.value(1)+" отправил(-а) файл - <i>"+fileName+"</i><br>  Размера <i>"+QString::number((double)flsz)+" байт</i>");
                } else if (fileSize < 1024*1024)    //килобайты
                {
                    double flsz = fileSize/1024.;
                    if(ui->lineEditLogin->text() == strList.value(1))
                        ui->textEditMessages->append("<br>Вы отправили файл - <i>"+fileName+"</i><br>Размера <i>"+QString::number((double)flsz)+" Кбайт</i>");
                    else    ui->textEditMessages->append("<br>"+strList.value(1)+" отправил(-а) файл - <i>"+fileName+"</i><br>  Размера <i>"+QString::number((double)flsz)+" Кбайт</i>");
                } else if (fileSize < 1024*1024*1024)   //мегабайты
                {
                    double flsz = fileSize/(1024*1024.);
                    if(ui->lineEditLogin->text() == strList.value(1))
                        ui->textEditMessages->append("<br>Вы отправили файл - <i>"+fileName+"</i><br> Размера <i>"+QString::number((double)flsz)+" Мбайт</i>");
                    else    ui->textEditMessages->append("<br>"+strList.value(1)+" отправил(-а) файл - <i>"+fileName+"</i><br> Размера <i>"+QString::number((double)flsz)+" Мбайт</i>");
                }
                ui->downloadFile->setEnabled(true);
                ui->sendFile->setEnabled(true);
                ui->ButtonSend->setEnabled(true);
                ui->lineEditMessage->setEnabled(true);
            }
            else if(strList.value(0) == "file") //обработчик загрузки файла
            {
                ui->textEditMessages->append("<br>Загрузка файла началась...");
                ui->downloadFile->setEnabled(true);
                ui->sendFile->setEnabled(true);
                ui->ButtonSend->setEnabled(true);
                ui->lineEditMessage->setEnabled(true);
            }
        }
        else
        {
            //получение файла клиентом
            in.startTransaction();
            if(countInterruptGettingFile == 0)
            {
                ui->textEditMessages->append("<br>Идёт загрузка файла - <i>"+fileName+"</i> с сервера...");
                countInterruptGettingFile++;
            }
            in >> fileBlocks;
            if (!in.commitTransaction())    //ждём полной передачи файла через сокет
            {
                //qDebug() << "SERVER: tmpBlock - FAIL commitTransaction";
                return;
            }
            qDebug() << "read...........";
            QFile file(filePathSave);
            if(file.open(QIODevice::WriteOnly))
            {
                file.write(fileBlocks);
                qDebug() << "Data has been written to a file!";
                QString strFilePathSave = filePathSave.left(filePathSave.indexOf(fileName));
                ui->textEditMessages->append("Файл загружен с сервера и сохранен в " + strFilePathSave+"<br>");
            }
            else
            {
                qDebug() << "The file don't open to write a data!";
                return;
            }
            file.close();
            qDebug() << "CLIENT: END - bytesAvailable:" << tcpSocket->bytesAvailable();

            ui->downloadFile->setEnabled(true);
            ui->sendFile->setEnabled(true);
            ui->ButtonSend->setEnabled(true);
            ui->lineEditMessage->setEnabled(true);
            countInterruptGettingFile = 0;
            isDownloading = false;
        }
    }
    else
    {
        ui->textEditMessages->append("Read error!");
    }


}

void ChatClient::on_ButtonSend_clicked()
{
    QDataStream stream(tcpSocket);
    stream.setVersion(QDataStream::Qt_5_12);
    QString strForServer("msg;"+ui->lineEditLogin->text() +";"+ ui->lineEditMessage->text());
    stream << strForServer;
    ui->lineEditMessage->clear();   //очищаем поле ввода
}

void ChatClient::on_lineEditMessage_returnPressed()
{
    QDataStream stream(tcpSocket);
    stream.setVersion(QDataStream::Qt_5_12);
    QString strForServer("msg;"+ui->lineEditLogin->text() +";"+ ui->lineEditMessage->text());
    stream << strForServer;
    ui->lineEditMessage->clear();   //очищаем поле ввода
}

void ChatClient::sendToServer(QString str)
{
    dataTcp.clear();
    QDataStream out(&dataTcp,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << str;
    tcpSocket->write(dataTcp);
}
void ChatClient::sendToServer(QByteArray dataFile)
{
    tcpSocket->write(dataFile);
}


void ChatClient::on_sendFile_clicked()
{
    ui->sendFile->setEnabled(false);
    bool wasActiveButtonDownload = false;
    if(ui->downloadFile->isEnabled())   wasActiveButtonDownload = true;
    ui->downloadFile->setEnabled(false);
    ui->ButtonSend->setEnabled(false);
    ui->lineEditMessage->setEnabled(false);
    filePath = QFileDialog::getOpenFileName( 0, tr("Open Document"), QDir::homePath(), tr("All files (*.*)"), 0, QFileDialog::DontUseNativeDialog );
    //sendToServer("download file;");
    qDebug() << filePath;
    if(filePath != "")
    {
        QDataStream stream(tcpSocket);
        stream.setVersion(QDataStream::Qt_5_12);

        QFile file(filePath);
        QFileInfo fileInfo(file);
        fileSize = fileInfo.size();

        QString infoAboutFileToServer="download file;"+ui->lineEditLogin->text()+";"+fileInfo.fileName()+";"+QString::number((long)fileSize);
        qDebug() << "Инфа о загружаемом файле: " + infoAboutFileToServer;
        stream << infoAboutFileToServer;
        //sendToServer(infoAboutFileToServer);
        tcpSocket->waitForBytesWritten();
        sendFile = new QFile(filePath);
        sendFullFile();
    }
    else
    {
        ui->sendFile->setEnabled(true);
        if(wasActiveButtonDownload) ui->downloadFile->setEnabled(true);
        else ui->downloadFile->setEnabled(false);
        ui->ButtonSend->setEnabled(true);
        ui->lineEditMessage->setEnabled(true);
        QMessageBox::critical(NULL,QObject::tr("Ошибка"),tr("Не был выбран файл для отправки!"));
    }
}

void ChatClient::on_downloadFile_clicked()
{
    ui->downloadFile->setEnabled(false);
    ui->sendFile->setEnabled(false);
    ui->ButtonSend->setEnabled(false);
    ui->lineEditMessage->setEnabled(false);
    filePathSave = QFileDialog::getSaveFileName( 0,"Сохранить файл как", QDir::homePath()+"/Документы/"+fileName,"All files (*.*)", 0, QFileDialog::DontUseNativeDialog );

    qDebug() << filePathSave;
    if(filePathSave != "")
    {
        QDataStream stream(tcpSocket);
        stream.setVersion(QDataStream::Qt_5_12);

        QString infoAboutFileFromServer = "get file;"+ui->lineEditLogin->text()+";"+fileName;
        qDebug() << "Инфа о скачиваемом файле: " + infoAboutFileFromServer;
        isDownloading = true;
        stream << infoAboutFileFromServer;
        //sendToServer(infoAboutFileToServer);
        tcpSocket->waitForBytesWritten();
    }
    else
    {
        QMessageBox::critical(NULL,QObject::tr("Ошибка"),tr("Не была выбрана папка для сохранения!"));
        ui->sendFile->setEnabled(true);
        ui->downloadFile->setEnabled(true);
        ui->ButtonSend->setEnabled(true);
        ui->lineEditMessage->setEnabled(true);
    }
}

void ChatClient::sendFullFile()
{
    //QThread::msleep(100);
    QDataStream stream(tcpSocket);
    stream.setVersion(QDataStream::Qt_5_12);

    if(sendFile->open(QFile::ReadOnly))
    {
        QByteArray dataS = sendFile->readAll();
        stream << dataS;
        tcpSocket->waitForBytesWritten();
        qDebug() << "_CLIENT: File end!";
        sendFile->close();
        sendFile = NULL;
        qDebug() << "_CLIENT: File send FINISH!";
    } else {
        qFatal("_CLIENT: File not open!");
    }

}

void ChatClient::slotSockDisc()
{
    qDebug() << "Deleting tcpSocket...";
    tcpSocket->deleteLater();
}


void ChatClient::displayError(QAbstractSocket::SocketError socketError) {
    switch (socketError) {
        case QAbstractSocket::RemoteHostClosedError:
            break;
        case QAbstractSocket::HostNotFoundError:
            qDebug() << "The host was not found. Please check the host name and port settings.";
            break;
        case QAbstractSocket::ConnectionRefusedError:
            qDebug() << "The connection was refused by the peer.";
            qDebug() << "Make sure the fortune server is running, ";
            qDebug() << "and check that the host name and port settings are correct.";
            break;
        default:
            qDebug() << "The following error occurred: " + QString("%1.").arg(tcpSocket->errorString());
    }
    exit(socketError + 1);
}






