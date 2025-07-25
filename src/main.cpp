#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("Ongaku");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Ongaku");

    MainWindow window;
    window.show();

    return app.exec();
}
