#include "MainWindow.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QColor>
#include <QHeaderView>
#include <QInputDialog>
#include <QMap>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QScreen>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "ResultExporter.h"

namespace {

// 录入对话框下拉框里的两个特殊项
const int kUnselected = -1;   // 还没选
const int kNoBuyer    = -2;   // 无人竞拍（流拍）

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

/// 带"万元"单位的金额文本
QString wanText(qint64 yuan)
{
    return QStringLiteral("%1 万元").arg(wanNumber(yuan));
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_index(0),
      m_auctionRunning(false),
      m_stack(nullptr),
      m_preparePage(nullptr),
      m_auctionPage(nullptr),
      m_resultPage(nullptr),
      m_eventLabel(nullptr),
      m_summaryLabel(nullptr),
      m_fileLabel(nullptr),
      m_chooseFileButton(nullptr),
      m_startButton(nullptr),
      m_previewTable(nullptr),
      m_progressLabel(nullptr),
      m_itemNameLabel(nullptr),
      m_itemDescLabel(nullptr),
      m_startPriceLabel(nullptr),
      m_previousButton(nullptr),
      m_modifyButton(nullptr),
      m_recordButton(nullptr),
      m_resultTitleLabel(nullptr),
      m_resultTable(nullptr),
      m_statLabel(nullptr),
      m_classStatLabel(nullptr),
      m_exportButton(nullptr),
      m_restartButton(nullptr)
{
    setWindowTitle(QStringLiteral("人生拍卖会 · 拍卖结果记录"));
    resize(1120, 780);

    buildUi();
    applyStyle();

    // 启动时按"可执行文件同目录 → 应用数据目录"的顺序自动找数据文件
    const QString defaultFile = AuctionData::locateDefaultFile();
    if (!defaultFile.isEmpty())
        loadDataFile(defaultFile, false);

    refreshPreparePage();

    // 窗口在第一个屏幕上居中
    if (QScreen *screen = QGuiApplication::primaryScreen())
    {
        const QRect available = screen->availableGeometry();
        move(available.center() - QPoint(width() / 2, height() / 2));
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::buildUi()
{
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    m_preparePage = createPreparePage();
    m_auctionPage = createAuctionPage();
    m_resultPage = createResultPage();

    m_stack->addWidget(m_preparePage);
    m_stack->addWidget(m_auctionPage);
    m_stack->addWidget(m_resultPage);
    m_stack->setCurrentWidget(m_preparePage);
}

QWidget *MainWindow::createPreparePage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *root = new QVBoxLayout(page);
    root->setContentsMargins(32, 26, 32, 24);
    root->setSpacing(16);

    m_eventLabel = new QLabel(QStringLiteral("人生拍卖会"));
    m_eventLabel->setObjectName(QStringLiteral("PageTitle"));
    m_eventLabel->setAlignment(Qt::AlignCenter);

    m_summaryLabel = new QLabel;
    m_summaryLabel->setObjectName(QStringLiteral("SubTitle"));
    m_summaryLabel->setAlignment(Qt::AlignCenter);
    m_summaryLabel->setWordWrap(true);

    QHBoxLayout *fileRow = new QHBoxLayout;
    fileRow->setSpacing(12);

    m_fileLabel = new QLabel;
    m_fileLabel->setObjectName(QStringLiteral("FileHint"));
    m_fileLabel->setWordWrap(true);
    m_fileLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_chooseFileButton = new QPushButton(QStringLiteral("选择数据文件…"));
    m_chooseFileButton->setObjectName(QStringLiteral("SecondaryButton"));
    m_chooseFileButton->setMinimumHeight(42);
    connect(m_chooseFileButton, &QPushButton::clicked, this, &MainWindow::onChooseFile);

    fileRow->addWidget(m_fileLabel, 1);
    fileRow->addWidget(m_chooseFileButton, 0);

    QLabel *previewTitle = new QLabel(QStringLiteral("拍品预览"));
    previewTitle->setObjectName(QStringLiteral("SectionTitle"));

    m_previewTable = new QTableWidget(0, 3);
    m_previewTable->setHorizontalHeaderLabels(
        QStringList() << QStringLiteral("序号") << QStringLiteral("拍品名称")
                      << QStringLiteral("起拍价（万元）"));
    m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_previewTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_previewTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_previewTable->setAlternatingRowColors(true);
    m_previewTable->verticalHeader()->setVisible(false);
    m_previewTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_previewTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_previewTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_previewTable->verticalHeader()->setDefaultSectionSize(40);

    QHBoxLayout *actionRow = new QHBoxLayout;
    m_startButton = new QPushButton(QStringLiteral("开始拍卖"));
    m_startButton->setObjectName(QStringLiteral("PrimaryButton"));
    m_startButton->setMinimumSize(220, 56);
    m_startButton->setEnabled(false);
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::onStartAuction);

    actionRow->addStretch(1);
    actionRow->addWidget(m_startButton);

    root->addWidget(m_eventLabel);
    root->addWidget(m_summaryLabel);
    root->addLayout(fileRow);
    root->addWidget(previewTitle);
    root->addWidget(m_previewTable, 1);
    root->addLayout(actionRow);

    return page;
}

QWidget *MainWindow::createAuctionPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *root = new QVBoxLayout(page);
    root->setContentsMargins(40, 26, 40, 26);
    root->setSpacing(12);

    m_progressLabel = new QLabel;
    m_progressLabel->setObjectName(QStringLiteral("SubTitle"));
    m_progressLabel->setAlignment(Qt::AlignCenter);

    m_itemNameLabel = new QLabel;
    m_itemNameLabel->setObjectName(QStringLiteral("ItemName"));
    m_itemNameLabel->setAlignment(Qt::AlignCenter);
    m_itemNameLabel->setWordWrap(true);

    m_itemDescLabel = new QLabel;
    m_itemDescLabel->setObjectName(QStringLiteral("ItemDesc"));
    m_itemDescLabel->setAlignment(Qt::AlignCenter);
    m_itemDescLabel->setWordWrap(true);

    m_startPriceLabel = new QLabel;
    m_startPriceLabel->setObjectName(QStringLiteral("StartPrice"));
    m_startPriceLabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *actionRow = new QHBoxLayout;
    actionRow->setSpacing(14);

    m_previousButton = new QPushButton(QStringLiteral("上一件"));
    m_previousButton->setObjectName(QStringLiteral("SecondaryButton"));
    m_previousButton->setMinimumSize(140, 52);
    m_previousButton->setEnabled(false);
    connect(m_previousButton, &QPushButton::clicked, this, &MainWindow::onPreviousItem);

    m_modifyButton = new QPushButton(QStringLiteral("修改结果"));
    m_modifyButton->setObjectName(QStringLiteral("SecondaryButton"));
    m_modifyButton->setMinimumSize(180, 52);
    connect(m_modifyButton, &QPushButton::clicked, this, &MainWindow::onModifyResult);

    m_recordButton = new QPushButton(QStringLiteral("录入结果"));
    m_recordButton->setObjectName(QStringLiteral("PrimaryButton"));
    m_recordButton->setMinimumSize(220, 52);
    connect(m_recordButton, &QPushButton::clicked, this, &MainWindow::onRecordResult);

    actionRow->addWidget(m_previousButton);
    actionRow->addStretch(1);
    actionRow->addWidget(m_modifyButton);
    actionRow->addWidget(m_recordButton);

    root->addWidget(m_progressLabel);
    root->addWidget(m_itemNameLabel);
    root->addWidget(m_itemDescLabel);
    root->addWidget(m_startPriceLabel);
    root->addStretch(1);
    root->addLayout(actionRow);

    return page;
}

QWidget *MainWindow::createResultPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *root = new QVBoxLayout(page);
    root->setContentsMargins(32, 24, 32, 24);
    root->setSpacing(14);

    m_resultTitleLabel = new QLabel(QStringLiteral("拍卖结果"));
    m_resultTitleLabel->setObjectName(QStringLiteral("PageTitle"));
    m_resultTitleLabel->setAlignment(Qt::AlignCenter);

    m_resultTable = new QTableWidget(0, 6);
    m_resultTable->setHorizontalHeaderLabels(
        QStringList() << QStringLiteral("序号") << QStringLiteral("拍品名称")
                      << QStringLiteral("起拍价（万元）") << QStringLiteral("成交价（万元）")
                      << QStringLiteral("买家") << QStringLiteral("状态"));
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->verticalHeader()->setVisible(false);
    m_resultTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    for (int col = 0; col < 6; ++col)
    {
        if (col != 1)
            m_resultTable->horizontalHeader()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
    }
    m_resultTable->verticalHeader()->setDefaultSectionSize(38);

    m_statLabel = new QLabel;
    m_statLabel->setObjectName(QStringLiteral("StatLine"));
    m_statLabel->setWordWrap(true);

    m_classStatLabel = new QLabel;
    m_classStatLabel->setObjectName(QStringLiteral("StatLine"));
    m_classStatLabel->setWordWrap(true);

    m_exportButton = new QPushButton(QStringLiteral("导出 CSV"));
    m_exportButton->setObjectName(QStringLiteral("PrimaryButton"));
    m_exportButton->setMinimumSize(180, 50);
    connect(m_exportButton, &QPushButton::clicked, this, &MainWindow::onExportCsv);

    m_restartButton = new QPushButton(QStringLiteral("重新开始"));
    m_restartButton->setObjectName(QStringLiteral("SecondaryButton"));
    m_restartButton->setMinimumSize(160, 50);
    connect(m_restartButton, &QPushButton::clicked, this, &MainWindow::onRestart);

    QHBoxLayout *buttonRow = new QHBoxLayout;
    buttonRow->addStretch(1);
    buttonRow->addWidget(m_restartButton);
    buttonRow->addWidget(m_exportButton);

    root->addWidget(m_resultTitleLabel);
    root->addWidget(m_resultTable, 1);
    root->addWidget(m_statLabel);
    root->addWidget(m_classStatLabel);
    root->addLayout(buttonRow);

    return page;
}

void MainWindow::applyStyle()
{
    // 字号整体偏大、对比度高，方便投影到教室大屏
    const QString style = QStringLiteral(
        "QWidget { background-color: #f4f6f9; color: #1f2329; font-size: 15px; }"
        "QLabel#PageTitle { font-size: 34px; font-weight: bold; color: #0d2f5c; padding: 4px; }"
        "QLabel#SubTitle { font-size: 18px; color: #4a5568; }"
        "QLabel#SectionTitle { font-size: 17px; font-weight: bold; color: #2d3748; padding-top: 6px; }"
        "QLabel#FileHint { font-size: 14px; color: #5a6672; }"
        "QLabel#ItemName { font-size: 80px; font-weight: bold; color: #0d2f5c; }"
        "QLabel#ItemDesc { font-size: 40px; color: #4a5568; }"
        "QLabel#StartPrice { font-size: 36px; font-weight: bold; color: #b26a00; }"
        "QLabel#StatLine { font-size: 10px; color: #2d3748; }"
        "QPushButton { border: none; border-radius: 8px; padding: 10px 24px; font-size: 17px; }"
        "QPushButton#PrimaryButton { background-color: #1565c0; color: #ffffff; font-weight: bold; }"
        "QPushButton#PrimaryButton:hover { background-color: #1e7ad4; }"
        "QPushButton#PrimaryButton:pressed { background-color: #0d47a1; }"
        "QPushButton#PrimaryButton:disabled { background-color: #b7c4d1; color: #eef2f6; }"
        "QPushButton#SecondaryButton { background-color: #ffffff; color: #1565c0;"
        "  border: 2px solid #1565c0; font-weight: bold; }"
        "QPushButton#SecondaryButton:hover { background-color: #e8f1fb; }"
        "QPushButton#SecondaryButton:disabled { color: #9aa7b4; border-color: #c3ccd6; }"
        "QTableWidget { background-color: #ffffff; border: 1px solid #d3dae3;"
        "  border-radius: 6px; gridline-color: #e4e9ef; }"
        "QTableWidget::item { padding: 4px 8px; }"
        "QTableWidget::item:selected { background-color: #d6e6f8; color: #1f2329; }"
        "QHeaderView::section { background-color: #e9eef4; color: #2d3748; font-weight: bold;"
        "  padding: 8px; border: none; border-bottom: 1px solid #d3dae3; }"
    );
    setStyleSheet(style);
}

bool MainWindow::loadDataFile(const QString &filePath, bool interactive)
{
    QString error;
    if (!m_data.loadFromFile(filePath, &error))
    {
        m_records.clear();
        m_index = 0;
        m_auctionRunning = false;
        refreshPreparePage();

        if (interactive)
        {
            QMessageBox box(this);
            box.setIcon(QMessageBox::Critical);
            box.setWindowTitle(QStringLiteral("数据加载失败"));
            box.setText(error);
            box.setInformativeText(QStringLiteral("可以点击「选择数据文件…」换一个文件。"));
            box.exec();
        }
        return false;
    }

    // 每次成功加载都重置运行状态，避免旧记录串到新数据上
    m_records = QVector<AuctionRecord>(m_data.items().size());
    m_index = 0;
    m_auctionRunning = false;
    refreshPreparePage();
    return true;
}

void MainWindow::refreshPreparePage()
{
    const bool loaded = m_data.isLoaded();

    if (loaded)
    {
        m_eventLabel->setText(m_data.eventName());
        m_summaryLabel->setText(QStringLiteral("共 %1 件拍品　｜　%2 个参与者")
                                    .arg(m_data.items().size())
                                    .arg(m_data.classNames().size()));
        m_fileLabel->setText(QStringLiteral("数据文件：%1")
                                 .arg(QDir::toNativeSeparators(m_data.sourcePath())));
    }
    else
    {
        m_eventLabel->setText(QStringLiteral("尚未加载数据"));
        m_summaryLabel->setText(QStringLiteral("请先选择一份 auction_data.json 数据文件"));
        m_fileLabel->setText(QStringLiteral(
            "自动查找顺序：① 可执行文件同目录（data/ 或程序旁边）"
            "　② 应用数据目录（AppDataLocation）　③ 手动选择"));
    }

    m_previewTable->setRowCount(0);
    if (loaded)
    {
        const QVector<AuctionItem> &items = m_data.items();
        m_previewTable->setRowCount(items.size());
        for (int row = 0; row < items.size(); ++row)
        {
            const AuctionItem &item = items.at(row);

            QTableWidgetItem *noItem = new QTableWidgetItem(QString::number(row + 1));
            noItem->setTextAlignment(Qt::AlignCenter);

            QTableWidgetItem *nameItem = new QTableWidgetItem(item.name);
            if (!item.description.isEmpty())
                nameItem->setToolTip(item.description);

            QTableWidgetItem *priceItem = new QTableWidgetItem(wanNumber(item.startPrice));
            priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

            m_previewTable->setItem(row, 0, noItem);
            m_previewTable->setItem(row, 1, nameItem);
            m_previewTable->setItem(row, 2, priceItem);
        }
    }

    m_startButton->setEnabled(loaded);
}

void MainWindow::onChooseFile()
{
    QString startDir = QDir::homePath();
    if (m_data.isLoaded())
        startDir = QFileInfo(m_data.sourcePath()).absolutePath();

    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择拍卖数据文件"), startDir,
        QStringLiteral("JSON 数据文件 (*.json);;所有文件 (*)"));

    if (path.isEmpty())
        return;

    loadDataFile(path, true);
}

void MainWindow::onStartAuction()
{
    if (!m_data.isLoaded())
    {
        QMessageBox::warning(this, QStringLiteral("无法开始"),
                             QStringLiteral("还没有加载数据文件。"));
        return;
    }
    if (m_auctionRunning)
        return;   // 防止重复进入

    m_records = QVector<AuctionRecord>(m_data.items().size());
    m_index = 0;
    m_auctionRunning = true;

    m_stack->setCurrentWidget(m_auctionPage);
    showItem(m_index);
}

void MainWindow::showItem(int index)
{
    const QVector<AuctionItem> &items = m_data.items();
    if (index < 0 || index >= items.size())
        return;

    const AuctionItem &item = items.at(index);
    m_progressLabel->setText(
        QStringLiteral("第 %1 / %2 件").arg(index + 1).arg(items.size()));
    m_itemNameLabel->setText(item.name);
    m_itemDescLabel->setText(item.description.isEmpty()
                                 ? QStringLiteral("（本件没有说明）")
                                 : item.description);
    m_startPriceLabel->setText(
        QStringLiteral("起拍价：%1").arg(wanText(item.startPrice)));
    m_previousButton->setEnabled(index > 0);

    bool hasRecorded = false;
    for (const AuctionRecord &record : m_records)
    {
        if (record.recorded)
        {
            hasRecorded = true;
            break;
        }
    }
    m_modifyButton->setEnabled(hasRecorded);
}

void MainWindow::onRecordResult()
{
    if (!m_auctionRunning)
        return;

    if (askForRecord(m_index))
        goToNextItem();
}

void MainWindow::onModifyResult()
{
    if (!m_auctionRunning)
        return;

    QStringList choices;
    QVector<int> indexes;
    const QVector<AuctionItem> &items = m_data.items();

    for (int i = 0; i < items.size() && i < m_records.size(); ++i)
    {
        const AuctionRecord &record = m_records.at(i);
        if (!record.recorded)
            continue;

        const QString resultText = record.sold
            ? QStringLiteral("%1 · %2").arg(wanText(record.finalPrice), record.buyerClass)
            : QStringLiteral("流拍");
        choices << QStringLiteral("第 %1 件：%2（%3）")
                       .arg(i + 1)
                       .arg(items.at(i).name)
                       .arg(resultText);
        indexes << i;
    }

    if (choices.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("暂无结果"),
                                 QStringLiteral("还没有录入任何拍卖结果。"));
        return;
    }

    bool ok = false;
    const QString choice = QInputDialog::getItem(
        this, QStringLiteral("修改结果"), QStringLiteral("请选择要修改的拍品："),
        choices, 0, false, &ok);
    if (!ok || choice.isEmpty())
        return;

    const int choiceIndex = choices.indexOf(choice);
    if (choiceIndex < 0)
        return;
    askForRecord(indexes.at(choiceIndex));
}

bool MainWindow::askForRecord(int index)
{
    const QVector<AuctionItem> &items = m_data.items();
    if (index < 0 || index >= items.size() || index >= m_records.size())
        return false;

    const AuctionItem &item = items.at(index);
    const AuctionRecord existing = m_records.at(index);

    QDialog dialog(this);
    dialog.setWindowTitle(existing.recorded ? QStringLiteral("修改拍卖结果")
                                            : QStringLiteral("录入拍卖结果"));
    dialog.setModal(true);
    dialog.setMinimumWidth(480);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 22, 26, 20);
    layout->setSpacing(14);

    QLabel *title = new QLabel(
        QStringLiteral("第 %1 件：%2").arg(index + 1).arg(item.name));
    title->setObjectName(QStringLiteral("SectionTitle"));
    title->setWordWrap(true);

    QLabel *hint = new QLabel(
        QStringLiteral("起拍价 %1，成交价不得低于起拍价。").arg(wanText(item.startPrice)));
    hint->setObjectName(QStringLiteral("FileHint"));
    hint->setWordWrap(true);

    QLineEdit *priceEdit = new QLineEdit;
    priceEdit->setMinimumHeight(40);
    priceEdit->setPlaceholderText(
        QStringLiteral("请输入万元金额，例如 %1").arg(wanNumber(item.startPrice)));
    priceEdit->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("\\d{0,9}(\\.\\d{0,4})?")), priceEdit));

    QComboBox *classBox = new QComboBox;
    classBox->setMinimumHeight(40);
    classBox->addItem(QStringLiteral("— 请选择买家 —"), kUnselected);
    for (int i = 0; i < m_data.classNames().size(); ++i)
        classBox->addItem(m_data.classNames().at(i), i);
    classBox->addItem(QStringLiteral("无人竞拍（流拍）"), kNoBuyer);

    QFormLayout *form = new QFormLayout;
    form->setSpacing(10);
    form->addRow(QStringLiteral("买家"), classBox);
    form->addRow(QStringLiteral("成交价（万元）"), priceEdit);

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    if (QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok))
        okButton->setText(QStringLiteral("确认"));
    if (QPushButton *cancelButton = buttonBox->button(QDialogButtonBox::Cancel))
        cancelButton->setText(QStringLiteral("取消"));

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // 回退修改时，把上一次录入的值直接填回去
    if (existing.recorded)
    {
        if (existing.sold)
        {
            const int classIdx = m_data.classNames().indexOf(existing.buyerClass);
            if (classIdx >= 0)
                classBox->setCurrentIndex(classIdx + 1);
            priceEdit->setText(wanNumber(existing.finalPrice));
        }
        else
        {
            classBox->setCurrentIndex(classBox->count() - 1);   // 无人竞拍
        }
    }

    // 选了"无人竞拍"就禁用价格输入，避免录入无意义的数字
    QObject::connect(classBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     &dialog, [priceEdit, classBox](int) {
                         const int choice = classBox->currentData().toInt();
                         priceEdit->setEnabled(choice >= 0);
                     });
    priceEdit->setEnabled(classBox->currentData().toInt() >= 0);

    layout->addWidget(title);
    layout->addWidget(hint);
    layout->addLayout(form);
    layout->addWidget(buttonBox);

    priceEdit->setFocus();

    while (dialog.exec() == QDialog::Accepted)
    {
        const int choice = classBox->currentData().toInt();

        if (choice == kUnselected)
        {
            QMessageBox::warning(&dialog, QStringLiteral("还不能确认"),
                                 QStringLiteral("请选择买家；如果没人出价，请选「无人竞拍（流拍）」。"));
            classBox->setFocus();
            continue;
        }

        AuctionRecord record;
        record.recorded = true;

        if (choice == kNoBuyer)
        {
            record.sold = false;
            record.finalPrice = 0;
            record.buyerClass.clear();
        }
        else
        {
            const QString text = priceEdit->text().trimmed();
            bool ok = false;
            const double wan = text.toDouble(&ok);

            if (text.isEmpty() || !ok)
            {
                QMessageBox::warning(&dialog, QStringLiteral("成交价无效"),
                                     QStringLiteral("请输入万元金额，最多 4 位小数。"));
                priceEdit->setFocus();
                priceEdit->selectAll();
                continue;
            }

            // 界面按万元录入，内部仍以元保存，避免浮点误差进入数据层
            const qint64 price = static_cast<qint64>(wan * 10000.0 + 0.5);
            if (price < item.startPrice)
            {
                QMessageBox::warning(&dialog, QStringLiteral("成交价过低"),
                                     QStringLiteral("成交价不能低于起拍价 %1。")
                                         .arg(wanText(item.startPrice)));
                priceEdit->setFocus();
                priceEdit->selectAll();
                continue;
            }

            record.sold = true;
            record.finalPrice = price;
            record.buyerClass = m_data.classNames().value(choice);
        }

        m_records[index] = record;
        return true;
    }

    return false;
}

void MainWindow::goToNextItem()
{
    if (m_index + 1 >= m_data.items().size())
    {
        finishAuction();
        return;
    }

    ++m_index;
    showItem(m_index);
}

void MainWindow::onPreviousItem()
{
    if (m_index <= 0)
        return;

    --m_index;
    showItem(m_index);

    // 回退后直接把原值弹出来，改完继续往下走
    if (askForRecord(m_index))
        goToNextItem();
}

void MainWindow::finishAuction()
{
    m_auctionRunning = false;
    refreshResultPage();
    m_stack->setCurrentWidget(m_resultPage);
}

void MainWindow::refreshResultPage()
{
    const QVector<AuctionItem> &items = m_data.items();

    m_resultTitleLabel->setText(QStringLiteral("%1 · 拍卖结果").arg(m_data.eventName()));

    m_resultTable->setRowCount(items.size());

    int soldCount = 0;
    int unsoldCount = 0;
    qint64 totalAmount = 0;
    QMap<QString, int> classCounts;

    for (int row = 0; row < items.size(); ++row)
    {
        const AuctionItem &item = items.at(row);
        const AuctionRecord record =
            (row < m_records.size()) ? m_records.at(row) : AuctionRecord();
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

        QTableWidgetItem *noItem = new QTableWidgetItem(QString::number(row + 1));
        noItem->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem *nameItem = new QTableWidgetItem(item.name);

        QTableWidgetItem *startItem = new QTableWidgetItem(wanNumber(item.startPrice));
        startItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QTableWidgetItem *finalItem =
            new QTableWidgetItem(sold ? wanNumber(record.finalPrice) : QStringLiteral("—"));
        finalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QTableWidgetItem *buyerItem =
            new QTableWidgetItem(sold ? record.buyerClass : QStringLiteral("—"));
        buyerItem->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem *statusItem =
            new QTableWidgetItem(sold ? QStringLiteral("成交") : QStringLiteral("流拍"));
        statusItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setForeground(sold ? QColor(QStringLiteral("#1b7f3b"))
                                       : QColor(QStringLiteral("#8a4b00")));

        m_resultTable->setItem(row, 0, noItem);
        m_resultTable->setItem(row, 1, nameItem);
        m_resultTable->setItem(row, 2, startItem);
        m_resultTable->setItem(row, 3, finalItem);
        m_resultTable->setItem(row, 4, buyerItem);
        m_resultTable->setItem(row, 5, statusItem);
    }

    m_statLabel->setText(QStringLiteral("成交 %1 件　｜　流拍 %2 件　｜　总成交金额 %3")
                             .arg(soldCount)
                             .arg(unsoldCount)
                             .arg(wanText(totalAmount)));

    if (classCounts.isEmpty())
    {
        m_classStatLabel->setText(QStringLiteral("各参与者拍得件数：本场没有成交记录"));
    }
    else
    {
        QStringList parts;
        // 按人员列表顺序输出，读起来和名册一致
        for (const QString &name : m_data.classNames())
        {
            const int count = classCounts.value(name, 0);
            parts << QStringLiteral("%1 %2 件").arg(name).arg(count);
        }
        m_classStatLabel->setText(QStringLiteral("各参与者拍得件数：%1")
                                      .arg(parts.join(QStringLiteral("　｜　"))));
    }
}

void MainWindow::onExportCsv()
{
    if (m_records.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("暂无数据"),
                                 QStringLiteral("还没有可导出的拍卖结果。"));
        return;
    }

    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (baseDir.isEmpty())
        baseDir = QDir::homePath();

    const QString suggested =
        QDir(baseDir).filePath(ResultExporter::defaultFileName(m_data.eventName()));

    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出拍卖结果"), suggested,
        QStringLiteral("CSV 文件 (*.csv);;所有文件 (*)"));

    if (path.isEmpty())
        return;

    if (!path.endsWith(QStringLiteral(".csv"), Qt::CaseInsensitive))
        path += QStringLiteral(".csv");

    QString error;
    if (ResultExporter::exportCsv(path, m_data.eventName(), m_data.items(), m_records,
                                  m_data.classNames(), &error))
    {
        QMessageBox::information(this, QStringLiteral("导出成功"),
                                 QStringLiteral("拍卖结果已保存到：\n%1")
                                     .arg(QDir::toNativeSeparators(path)));
    }
    else
    {
        QMessageBox::critical(this, QStringLiteral("导出失败"), error);
    }
}

void MainWindow::onRestart()
{
    if (m_auctionRunning)
    {
        const QMessageBox::StandardButton ret = QMessageBox::question(
            this, QStringLiteral("重新开始"),
            QStringLiteral("当前拍卖还没结束，重新开始会清空已录入的结果。要继续吗？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes)
            return;
    }

    m_records = QVector<AuctionRecord>(m_data.items().size());
    m_index = 0;
    m_auctionRunning = false;

    refreshPreparePage();
    m_stack->setCurrentWidget(m_preparePage);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_auctionRunning)
    {
        const QMessageBox::StandardButton ret = QMessageBox::question(
            this, QStringLiteral("退出确认"),
            QStringLiteral("拍卖尚未完成，确定要退出吗？\n未导出的结果不会保存。"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes)
        {
            event->ignore();
            return;
        }
    }

    event->accept();
}

