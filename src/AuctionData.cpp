#include "AuctionData.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include <string>

#include "CJsonObject.hpp"

namespace {

QString fromStd(const std::string &s)
{
    return QString::fromUtf8(s.c_str(), static_cast<int>(s.size()));
}

} // namespace

AuctionData::AuctionData()
    : m_loaded(false)
{
}

void AuctionData::clear()
{
    m_loaded = false;
    m_sourcePath.clear();
    m_eventName.clear();
    m_items.clear();
    m_classNames.clear();
}

/**
 * 默认查找顺序：
 *   ① 可执行文件同目录（先找 data 子目录，再找可执行文件旁边）
 *   ② macOS 应用包内的 Resources 目录（打成 .app 时用得上）
 *   ③ QStandardPaths::AppDataLocation（用户数据目录）
 * 全部用 QDir 拼接，不在任何地方硬编码分隔符。
 */
QStringList AuctionData::candidatePaths()
{
    QStringList paths;
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString fileName = QStringLiteral("auction_data.json");

    if (!appDir.isEmpty())
    {
        QDir dir(appDir);
        paths << QDir::cleanPath(dir.filePath(QStringLiteral("data/") + fileName));
        paths << QDir::cleanPath(dir.filePath(fileName));
        // .app/Contents/MacOS/AuctionRecorder → ../Resources/auction_data.json
        paths << QDir::cleanPath(dir.filePath(QStringLiteral("../Resources/") + fileName));
    }

    const QStringList appDataDirs =
        QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    for (const QString &base : appDataDirs)
    {
        if (base.isEmpty())
            continue;
        paths << QDir::cleanPath(QDir(base).filePath(fileName));
    }

    paths.removeDuplicates();
    return paths;
}

QString AuctionData::locateDefaultFile()
{
    const QStringList paths = candidatePaths();
    for (const QString &path : paths)
    {
        const QFileInfo info(path);
        if (info.exists() && info.isFile())
            return info.absoluteFilePath();
    }
    return QString();
}

bool AuctionData::loadFromFile(const QString &filePath, QString *errorMsg)
{
    clear();

    if (errorMsg)
        errorMsg->clear();

    QString err;

    // ---- 1. 文件本身 ----
    const QFileInfo info(filePath);
    if (filePath.trimmed().isEmpty())
    {
        err = QStringLiteral("未指定数据文件路径。");
    }
    else if (!info.exists())
    {
        err = QStringLiteral("找不到文件：%1").arg(QDir::toNativeSeparators(filePath));
    }
    else if (!info.isFile())
    {
        err = QStringLiteral("该路径不是一个文件：%1")
                  .arg(QDir::toNativeSeparators(filePath));
    }

    if (err.isEmpty() && !info.isReadable())
        err = QStringLiteral("文件没有读取权限：%1")
                  .arg(QDir::toNativeSeparators(filePath));

    QByteArray raw;
    if (err.isEmpty())
    {
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly))
        {
            err = QStringLiteral("打开文件失败：%1")
                      .arg(QDir::toNativeSeparators(info.absoluteFilePath()));
        }
        else
        {
            raw = file.readAll();
            file.close();
            if (raw.isEmpty())
                err = QStringLiteral("文件内容为空：%1")
                          .arg(QDir::toNativeSeparators(info.absoluteFilePath()));
        }
    }

    if (!err.isEmpty())
    {
        if (errorMsg)
            *errorMsg = err;
        return false;
    }

    // 去掉 UTF-8 BOM，避免带 BOM 的文件解析失败
    if (raw.startsWith("\xEF\xBB\xBF"))
        raw.remove(0, 3);

    // ---- 2. JSON 解析（CJsonObject） ----
    neb::CJsonObject root;
    if (!root.Parse(std::string(raw.constData(), static_cast<size_t>(raw.size()))))
    {
        if (errorMsg)
        {
            // CJsonObject 的错误串形如 "prase json string error at <出错位置后的内容>"，
            // 位置片段常常为空，这里只把有内容的部分取出来
            const QString raw = fromStd(root.GetErrMsg());
            const int atPos = raw.indexOf(QStringLiteral(" at "));
            const QString position = (atPos >= 0) ? raw.mid(atPos + 4).trimmed() : QString();

            QString message = QStringLiteral("JSON 解析失败：文件格式不正确。");
            if (!position.isEmpty())
                message += QStringLiteral("\n出错位置附近的内容：%1").arg(position.left(60));
            message += QStringLiteral("\n\n请检查括号和逗号是否配对、字符串是否用英文双引号、"
                                      "文件是否为 UTF-8 编码。");
            *errorMsg = message;
        }
        return false;
    }

    if (root.IsArray())
    {
        if (errorMsg)
            *errorMsg = QStringLiteral("JSON 根节点必须是对象（以 { 开头），当前是数组。");
        return false;
    }

    // ---- 3. 逐字段校验，一次性把问题收集齐，方便用户一次改完 ----
    QStringList problems;

    std::string strValue;
    m_eventName = QStringLiteral("人生拍卖会");
    if (root.Get("eventName", strValue))
    {
        const QString name = fromStd(strValue).trimmed();
        if (!name.isEmpty())
            m_eventName = name;
    }

    // 拍品
    neb::CJsonObject itemsJson;
    if (!root.Get("items", itemsJson) || !itemsJson.IsArray())
    {
        problems << QStringLiteral("缺少 items 数组，或 items 不是数组。");
    }
    else if (itemsJson.GetArraySize() <= 0)
    {
        problems << QStringLiteral("items 数组为空，至少要有一件拍品。");
    }
    else
    {
        const int count = itemsJson.GetArraySize();
        for (int i = 0; i < count; ++i)
        {
            neb::CJsonObject itemJson;
            if (!itemsJson.Get(i, itemJson))
            {
                problems << QStringLiteral("第 %1 件拍品格式错误，应为对象。").arg(i + 1);
                continue;
            }

            AuctionItem item;
            item.id = i + 1;

            int64 idValue = 0;
            if (itemJson.Get("id", idValue))
                item.id = static_cast<int>(idValue);

            if (!itemJson.Get("name", strValue) || fromStd(strValue).trimmed().isEmpty())
            {
                problems << QStringLiteral("第 %1 件拍品缺少 name 字段或 name 为空。")
                                .arg(i + 1);
            }
            else
            {
                item.name = fromStd(strValue).trimmed();
            }

            if (itemJson.Get("description", strValue))
                item.description = fromStd(strValue).trimmed();

            double priceValue = 0.0;
            if (!itemJson.Get("startPrice", priceValue))
            {
                problems << QStringLiteral("拍品「%1」缺少 startPrice 字段或不是数字。")
                                .arg(item.name.isEmpty() ? QStringLiteral("第 %1 件").arg(i + 1)
                                                         : item.name);
            }
            else if (priceValue < 0.0)
            {
                problems << QStringLiteral("拍品「%1」的 startPrice 不能为负数。")
                                .arg(item.name.isEmpty() ? QStringLiteral("第 %1 件").arg(i + 1)
                                                         : item.name);
            }
            else
            {
                item.startPrice = static_cast<qint64>(priceValue + 0.5);
            }

            m_items.append(item);
        }
    }

    // 班级
    neb::CJsonObject classesJson;
    if (!root.Get("classes", classesJson) || !classesJson.IsArray())
    {
        problems << QStringLiteral("缺少 classes 数组，或 classes 不是数组。");
    }
    else if (classesJson.GetArraySize() <= 0)
    {
        problems << QStringLiteral("classes 数组为空，至少要有一个参赛班级。");
    }
    else
    {
        const int count = classesJson.GetArraySize();
        for (int i = 0; i < count; ++i)
        {
            neb::CJsonObject classJson;
            if (!classesJson.Get(i, classJson))
            {
                problems << QStringLiteral("第 %1 个班级格式错误，应为对象。").arg(i + 1);
                continue;
            }

            if (!classJson.Get("name", strValue) || fromStd(strValue).trimmed().isEmpty())
            {
                problems << QStringLiteral("第 %1 个班级缺少 name 字段或 name 为空。")
                                .arg(i + 1);
                continue;
            }

            const QString className = fromStd(strValue).trimmed();
            if (m_classNames.contains(className))
            {
                problems << QStringLiteral("班级名称重复：%1").arg(className);
                continue;
            }
            m_classNames.append(className);
        }
    }

    if (!problems.isEmpty())
    {
        // 数据不完整就直接丢弃，避免界面显示半截数据
        clear();

        if (errorMsg)
        {
            *errorMsg = QStringLiteral("数据文件存在 %1 处问题：\n\n· %2")
                            .arg(problems.size())
                            .arg(problems.join(QStringLiteral("\n· ")));
        }
        return false;
    }

    m_sourcePath = info.absoluteFilePath();
    m_loaded = true;
    return true;
}
