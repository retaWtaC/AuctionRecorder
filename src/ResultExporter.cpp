#include "ResultExporter.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>

namespace {

/// 元 → 万元，最多保留 4 位小数并去掉末尾多余的 0
QString wanNumber(qint64 yuan)
{
    const qint64 whole = yuan / 10000;
    const qint64 fraction = yuan % 10000;
    if (fraction == 0)
        return QString::number(whole);

    QString fractionText = QStringLiteral("%1").arg(fraction, 4, 10, QLatin1Char('0'));
    while (fractionText.endsWith(QLatin1Char('0')))
        fractionText.chop(1);
    return QStringLiteral("%1.%2").arg(whole).arg(fractionText);
}

} // namespace

QString ResultExporter::escapeField(const QString &field)
{
    // 含逗号、引号、换行的字段必须整体加引号，内部引号翻倍
    if (field.contains(QLatin1Char(',')) || field.contains(QLatin1Char('"'))
        || field.contains(QLatin1Char('\n')) || field.contains(QLatin1Char('\r')))
    {
        QString escaped = field;
        escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }
    return field;
}

QString ResultExporter::defaultFileName(const QString &eventName)
{
    QString base = eventName.trimmed();
    if (base.isEmpty())
        base = QStringLiteral("拍卖结果");

    // 去掉各平台文件系统不接受的字符，保证在三大平台上都能存盘
    const QString invalid = QStringLiteral("\\/:*?\"<>|");
    for (const QChar &ch : invalid)
        base.remove(ch);

    const QString stamp =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmm"));
    return QStringLiteral("%1_%2.csv").arg(base, stamp);
}

bool ResultExporter::exportCsv(const QString &filePath,
                               const QString &eventName,
                               const QVector<AuctionItem> &items,
                               const QVector<AuctionRecord> &records,
                               const QStringList &classNames,
                               QString *errorMsg)
{
    if (errorMsg)
        errorMsg->clear();

    if (filePath.trimmed().isEmpty())
    {
        if (errorMsg)
            *errorMsg = QStringLiteral("未指定导出路径。");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if (errorMsg)
        {
            *errorMsg = QStringLiteral("无法写入文件：%1\n%2")
                            .arg(QDir::toNativeSeparators(filePath), file.errorString());
        }
        return false;
    }

    // Windows 版 Excel 只有见到 UTF-8 BOM 才会按 UTF-8 解码，
    // 否则中文全变乱码；macOS / Linux 的表格软件也能正常忽略它。
    QByteArray out("\xEF\xBB\xBF");

    const QString eol = QStringLiteral("\r\n");

    out += escapeField(QStringLiteral("活动名称")).toUtf8();
    out += ',';
    out += escapeField(eventName).toUtf8();
    out += eol.toUtf8();

    out += escapeField(QStringLiteral("导出时间")).toUtf8();
    out += ',';
    out += escapeField(QDateTime::currentDateTime().toString(
                            QStringLiteral("yyyy-MM-dd HH:mm:ss"))).toUtf8();
    out += eol.toUtf8();
    out += eol.toUtf8();

    // ---- 明细表 ----
    out += QStringLiteral("序号,拍品名称,起拍价（万元）,成交价（万元）,买家班级,状态").toUtf8();
    out += eol.toUtf8();

    int soldCount = 0;
    int unsoldCount = 0;
    qint64 totalAmount = 0;
    QMap<QString, int> classCounts;

    const int rowCount = items.size();
    for (int i = 0; i < rowCount; ++i)
    {
        const AuctionItem &item = items.at(i);
        const AuctionRecord record = (i < records.size()) ? records.at(i) : AuctionRecord();

        const bool sold = record.recorded && record.sold;
        if (sold)
        {
            ++soldCount;
            totalAmount += record.finalPrice;
            classCounts[record.buyerClass] += 1;
        }
        else
        {
            ++unsoldCount;
        }

        QStringList cells;
        cells << QString::number(i + 1)
              << escapeField(item.name)
              << wanNumber(item.startPrice)
              << (sold ? wanNumber(record.finalPrice) : QStringLiteral("0"))
              << escapeField(sold ? record.buyerClass : QString())
              << (sold ? QStringLiteral("成交") : QStringLiteral("流拍"));

        out += cells.join(QLatin1Char(',')).toUtf8();
        out += eol.toUtf8();
    }

    // ---- 统计块 ----
    out += eol.toUtf8();
    out += QStringLiteral("统计项,数值").toUtf8();
    out += eol.toUtf8();

    const QStringList statsLines = QStringList()
        << QStringLiteral("成交件数,%1").arg(soldCount)
        << QStringLiteral("流拍件数,%1").arg(unsoldCount)
        << QStringLiteral("总成交金额（万元）,%1").arg(wanNumber(totalAmount))
        << QStringLiteral("拍品总数,%1").arg(rowCount);

    for (const QString &line : statsLines)
    {
        out += line.toUtf8();
        out += eol.toUtf8();
    }

    // 各班级拍得件数：先按班级列表顺序输出，再补上不认识的买家
    out += eol.toUtf8();
    out += QStringLiteral("买家班级,拍得件数").toUtf8();
    out += eol.toUtf8();

    QStringList ordered = classNames;
    for (auto it = classCounts.constBegin(); it != classCounts.constEnd(); ++it)
    {
        if (!ordered.contains(it.key()))
            ordered << it.key();
    }

    for (const QString &name : ordered)
    {
        if (classCounts.value(name, 0) <= 0)
            continue;
        out += escapeField(name).toUtf8();
        out += ',';
        out += QString::number(classCounts.value(name)).toUtf8();
        out += eol.toUtf8();
    }

    const qint64 written = file.write(out);
    if (written != out.size())
    {
        if (errorMsg)
        {
            *errorMsg = QStringLiteral("写入过程中出错：%1")
                            .arg(file.errorString());
        }
        file.close();
        return false;
    }

    if (!file.flush())
    {
        if (errorMsg)
            *errorMsg = QStringLiteral("刷新文件缓冲区失败：%1").arg(file.errorString());
        file.close();
        return false;
    }

    file.close();
    return true;
}
