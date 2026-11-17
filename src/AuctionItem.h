#ifndef AUCTIONITEM_H
#define AUCTIONITEM_H

#include <QString>
#include <QtGlobal>

/**
 * @brief 单件拍品的数据结构
 *
 * 由 AuctionData 从 JSON 解析后填充，界面层直接读取，不再做格式校验。
 */
struct AuctionItem
{
    int     id = 0;             ///< 拍品编号，JSON 里没写时用数组序号补上
    QString name;               ///< 拍品名称
    QString description;        ///< 拍品说明（可为空）
    qint64  startPrice = 0;     ///< 起拍价，单位：元
};

/**
 * @brief 单件拍品的拍卖结果
 *
 * recorded 单独存在的原因是："流拍"也是一种已经录入的结果，
 * 不能靠 sold / buyerClass 是否为空来判断"录没录过"。
 */
struct AuctionRecord
{
    bool    recorded = false;   ///< 是否已录入结果
    bool    sold = false;       ///< 是否成交
    qint64  finalPrice = 0;     ///< 成交价，流拍时为 0
    QString buyerClass;         ///< 买家班级，流拍时为空
};

#endif // AUCTIONITEM_H
