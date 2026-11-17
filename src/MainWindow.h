#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QVector>

#include "AuctionData.h"
#include "AuctionItem.h"

class QCloseEvent;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTableWidget;

/**
 * @brief 主窗口：三个页面（准备 / 拍卖 / 结果）共用一个 QStackedWidget
 *
 * 界面逻辑只读 AuctionData 暴露的容器，不直接接触 CJsonObject；
 * 结果写入通过 ResultExporter 完成，导出格式改动不影响界面。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onChooseFile();             // 选择数据文件
    void onStartAuction();           // 开始拍卖
    void onRecordResult();           // 录入当前拍品的结果
    void onModifyResult();           // 修改已经录入的结果
    void onPreviousItem();           // 上一件
    void onExportCsv();              // 导出 CSV
    void onRestart();                // 重新开始

private:
    void buildUi();
    QWidget *createPreparePage();
    QWidget *createAuctionPage();
    QWidget *createResultPage();
    void applyStyle();

    bool loadDataFile(const QString &filePath, bool interactive);
    void refreshPreparePage();

    void showItem(int index);

    bool askForRecord(int index);    // 弹出录入对话框，确认并写入返回 true
    void goToNextItem();
    void finishAuction();
    void refreshResultPage();

    // ---- 数据与运行状态 ----
    AuctionData m_data;
    QVector<AuctionRecord> m_records;
    int  m_index;                    // 当前拍品下标（0 起）
    bool m_auctionRunning;           // 拍卖进行中（未到结果页）

    // ---- 界面 ----
    QStackedWidget *m_stack;
    QWidget *m_preparePage;
    QWidget *m_auctionPage;
    QWidget *m_resultPage;

    QLabel *m_eventLabel;
    QLabel *m_summaryLabel;
    QLabel *m_fileLabel;
    QPushButton *m_chooseFileButton;
    QPushButton *m_startButton;
    QTableWidget *m_previewTable;

    QLabel *m_progressLabel;
    QLabel *m_itemNameLabel;
    QLabel *m_itemDescLabel;
    QLabel *m_startPriceLabel;
    QPushButton *m_previousButton;
    QPushButton *m_modifyButton;
    QPushButton *m_recordButton;

    QLabel *m_resultTitleLabel;
    QTableWidget *m_resultTable;
    QLabel *m_statLabel;
    QLabel *m_classStatLabel;
    QPushButton *m_exportButton;
    QPushButton *m_restartButton;
};

#endif // MAINWINDOW_H
