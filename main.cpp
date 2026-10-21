
#include "ABC3D.h"
#include <QtWidgets/QApplication>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Asset paths are relative ("assets/..."). An installed copy is launched from
    // shortcuts with an arbitrary working directory, so resolve them against the exe
    // when assets/ ships next to it. Dev runs keep the project dir from Visual Studio.
    QString appDir = QCoreApplication::applicationDirPath();
    if (QDir(appDir).exists("assets")) {
        QDir::setCurrent(appDir);
    }

    ABC3D window;
    window.showMaximized();
    return app.exec();
}
