#include <QCoreApplication>
#include <QDebug>
#include <QStringList>

#include "AuctionData.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    if (argc < 2)
    {
        qWarning() << "usage: test_data <json>";
        return 2;
    }

    AuctionData data;
    QString error;
    const bool ok = data.loadFromFile(QString::fromLocal8Bit(argv[1]), &error);

    qDebug().noquote() << "loaded =" << ok;
    if (!ok)
    {
        qDebug().noquote() << "error  =" << error;
        return 1;
    }

    qDebug().noquote() << "event  =" << data.eventName();
    qDebug().noquote() << "items  =" << data.items().size();
    qDebug().noquote() << "classes=" << data.classNames().join(QStringLiteral(", "));
    for (const AuctionItem &item : data.items())
        qDebug().noquote() << QStringLiteral("  #%1 %2 | %3 | %4")
                                  .arg(item.id)
                                  .arg(item.name)
                                  .arg(item.description)
                                  .arg(item.startPrice);
    return 0;
}
