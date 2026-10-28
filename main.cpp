#include "mainwindow.h"
#include <QApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(QStringLiteral("SimpleEditor"));
    a.setApplicationVersion(QStringLiteral("1.0"));

    const QStringList args = a.arguments();
    const bool isDemo = args.contains(QStringLiteral("--demo"));

    MainWindow w;
    /* --demo：窗口固定贴在屏幕左上角，便于录屏脚本按固定区域采集 */
    if (isDemo) {
        w.resize(1600, 900);
        w.move(0, 0);
    }
    w.show();

    /* --shoot <目录>：自动演示全部功能并截图（用于实验报告与回归自检） */
    const int shootIdx = args.indexOf(QStringLiteral("--shoot"));
    if (shootIdx >= 0) {
        const QString dir = (shootIdx + 1 < args.size()) ? args.at(shootIdx + 1)
                                                         : QStringLiteral("shots");
        QTimer::singleShot(600, &w, [&w, dir]() { w.shootDemo(dir); });
    }

    /* --demo：按时间轴真实操作界面，供外部录屏采集（录制测试视频） */
    if (isDemo) {
        QTimer::singleShot(900, &w, [&w]() { w.runLiveDemo(); });
    }

    return a.exec();
}
