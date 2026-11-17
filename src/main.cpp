#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QStringList>
#include <QtGlobal>

#include "MainWindow.h"

namespace {

/**
 * 挑一款当前系统上确实存在的中文字体。
 * 不写死某个平台专有字体名，找不到就交给 Qt 自己回退。
 */
void applyChineseFont(QApplication &app)
{
    const QStringList candidates = QStringList()
        << QStringLiteral("Microsoft YaHei UI")
        << QStringLiteral("Microsoft YaHei")
        << QStringLiteral("PingFang SC")
        << QStringLiteral("Hiragino Sans GB")
        << QStringLiteral("Noto Sans CJK SC")
        << QStringLiteral("Noto Sans SC")
        << QStringLiteral("Source Han Sans SC")
        << QStringLiteral("Source Han Sans CN")
        << QStringLiteral("WenQuanYi Micro Hei")
        << QStringLiteral("Droid Sans Fallback")
        << QStringLiteral("SimHei");

    const QStringList installed = QFontDatabase().families();

    QFont font = app.font();
    for (const QString &family : candidates)
    {
        if (installed.contains(family, Qt::CaseInsensitive))
        {
            font.setFamily(family);
            break;
        }
    }
    font.setPointSize(12);
    app.setFont(font);
}

} // namespace

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt5 需要手动打开高 DPI；Qt6 默认已开启，重复设置会告警
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif

    QApplication app(argc, argv);

    // QStandardPaths::AppDataLocation 依赖这几个名字，必须在建窗口前设置
    QCoreApplication::setOrganizationName(QStringLiteral("AuctionRecorder"));
    QCoreApplication::setApplicationName(QStringLiteral("AuctionRecorder"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    app.setApplicationDisplayName(QStringLiteral("人生拍卖会 · 拍卖结果记录"));

    applyChineseFont(app);

    MainWindow window;
    window.show();

    return app.exec();
}
