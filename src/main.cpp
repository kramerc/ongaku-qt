#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("VibeQt");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("VibeQt");

    MainWindow window;
    window.show();

    return app.exec();
}
