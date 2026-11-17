#ifndef RESULTEXPORTER_H
#define RESULTEXPORTER_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "AuctionItem.h"

/**
 * @brief 结果导出：把拍卖记录写成 Excel 能直接打开的 CSV
 */
class ResultExporter
{
public:
    /**
     * @brief 导出 CSV
     * @param filePath   目标文件路径（由用户通过 QFileDialog 选择）
     * @param eventName  活动名称，写进文件头
     * @param items      拍品列表
     * @param records    与 items 一一对应的拍卖结果
     * @param classNames 班级列表，用于统计各班级成交件数
     * @param errorMsg   失败时写入中文错误说明
     */
    static bool exportCsv(const QString &filePath,
                          const QString &eventName,
                          const QVector<AuctionItem> &items,
                          const QVector<AuctionRecord> &records,
                          const QStringList &classNames,
                          QString *errorMsg);

    /// 生成一个带时间戳的默认文件名，供保存对话框预填
    static QString defaultFileName(const QString &eventName);

private:
    static QString escapeField(const QString &field);
};

#endif // RESULTEXPORTER_H
