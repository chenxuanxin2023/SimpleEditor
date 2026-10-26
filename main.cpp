#include "mainwindow.h"
#include <QApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(QStringLiteral("SimpleEditor"));
    a.setApplicationVersion(QStringLiteral("1.0"));

    MainWindow w;
    w.show();

    /* --shoot <目录>：自动演示全部功能并截图（用于实验报告与回归自检） */
    const QStringList args = a.arguments();
    const int shootIdx = args.indexOf(QStringLiteral("--shoot"));
    if (shootIdx >= 0) {
        const QString dir = (shootIdx + 1 < args.size()) ? args.at(shootIdx + 1)
                                                         : QStringLiteral("shots");
        QTimer::singleShot(600, &w, [&w, dir]() { w.shootDemo(dir); });
    }

    return a.exec();
}
