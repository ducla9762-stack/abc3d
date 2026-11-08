
#include "ABC3D.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ABC3D window;
    window.showMaximized();
    return app.exec();
}
