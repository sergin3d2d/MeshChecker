#include "MainWindow.h"
#include "Logger.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Logger::getInstance().init("apparel_mesh_checker.log");

    MainWindow window;
    window.show();

    return app.exec();
}
