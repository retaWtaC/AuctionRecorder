#-------------------------------------------------------------------------------
# AuctionRecorder —— 人生拍卖会 · 拍卖结果记录（qmake 工程文件）
# 用法：qmake AuctionRecorder.pro && make   (Windows 下为 nmake / mingw32-make)
#-------------------------------------------------------------------------------

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = AuctionRecorder
TEMPLATE = app

# Qt6 的 qmake 需要 C++17，Qt5 用 C++11 即可
contains(QT_MAJOR_VERSION, 6) {
    CONFIG += c++17
} else {
    CONFIG += c++11
}

# 关闭 Qt 已废弃 API，及早暴露跨版本不兼容的用法
DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD/src $$PWD/third_party/CJsonObject

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/AuctionData.cpp \
    src/ResultExporter.cpp \
    third_party/CJsonObject/CJsonObject.cpp \
    third_party/CJsonObject/cJSON.c

HEADERS += \
    src/MainWindow.h \
    src/AuctionData.h \
    src/AuctionItem.h \
    src/ResultExporter.h \
    third_party/CJsonObject/CJsonObject.hpp \
    third_party/CJsonObject/cJSON.h

# MSVC 下源码按 UTF-8 解析，否则中文字面量乱码
win32-msvc*: QMAKE_CXXFLAGS += /utf-8
win32-msvc*: QMAKE_CFLAGS   += /utf-8

# Linux 下链接数学库（cJSON 用到 floor/pow 等）
unix:!macx: LIBS += -lm

# 构建后把示例数据复制到可执行文件旁边（Windows 不自动复制，见 README）
unix {
    QMAKE_POST_LINK += mkdir -p $$shell_path($$OUT_PWD/data) && cp -f $$shell_path($$PWD/data/auction_data.json) $$shell_path($$OUT_PWD/data/)
}
