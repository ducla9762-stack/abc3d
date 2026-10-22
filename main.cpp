
#include "ABC3D.h"
#include <QtWidgets/QApplication>
#include <QDir>
#include <QToolTip>
#include <QColor>
#include <QPalette>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QPalette toolTips = QToolTip::palette();
    toolTips.setColor(QPalette::ToolTipBase, Qt::red);
    toolTips.setColor(QPalette::ToolTipText, Qt::white);
    QToolTip::setPalette(toolTips);

    qDebug() << QToolTip::palette().color(QPalette::ToolTipBase).name();

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
