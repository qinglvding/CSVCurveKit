#include <QApplication>
#include <QFont>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置全局默认字体
    QFont defaultFont("Microsoft YaHei", 9);
    a.setFont(defaultFont);

    MainWindow w;
    w.show();
    
    return a.exec();
}
