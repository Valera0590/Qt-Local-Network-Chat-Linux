#include "udpclient.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ChatClient w;
    w.setWindowTitle("Chat Client");
    w.show();
    return a.exec();
}
