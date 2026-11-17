#ifndef AUCTIONDATA_H
#define AUCTIONDATA_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "AuctionItem.h"

/**
 * @brief 数据层：负责读取并校验 auction_data.json
 *
 * 内部使用 CJsonObject 解析，对外只暴露纯 Qt 容器，
 * 界面层因此完全不依赖 CJsonObject 与 cJSON。
 */
class AuctionData
{
public:
    AuctionData();

    /// 默认数据文件查找顺序，按优先级从高到低返回完整路径
    static QStringList candidatePaths();

    /// 返回第一个实际存在的候选路径，都不存在则返回空字符串
    static QString locateDefaultFile();

    /**
     * @brief 加载并校验数据文件
     * @param filePath   JSON 文件路径
     * @param errorMsg   失败时写入面向用户的中文错误说明
     * @return 成功返回 true；失败时内部数据保持清空状态
     */
    bool loadFromFile(const QString &filePath, QString *errorMsg);

    void clear();

    bool isLoaded() const { return m_loaded; }
    QString sourcePath() const { return m_sourcePath; }
    QString eventName() const { return m_eventName; }
    const QVector<AuctionItem> &items() const { return m_items; }
    const QStringList &classNames() const { return m_classNames; }

private:
    bool m_loaded;
    QString m_sourcePath;
    QString m_eventName;
    QVector<AuctionItem> m_items;
    QStringList m_classNames;
};

#endif // AUCTIONDATA_H
