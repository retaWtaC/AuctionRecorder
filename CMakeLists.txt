cmake_minimum_required(VERSION 3.16)

project(AuctionRecorder VERSION 1.0.0 LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Qt5 / Qt6 自动探测：优先找 Qt6，找不到再退回到 Qt5
# ---------------------------------------------------------------------------
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

message(STATUS "使用 Qt${QT_VERSION_MAJOR} (${QT_VERSION})")

# Qt6 强制要求 C++17；Qt5 用 C++11 即可
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    set(CMAKE_CXX_STANDARD 17)
else()
    set(CMAKE_CXX_STANDARD 11)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "构建类型" FORCE)
endif()

set(PROJECT_SOURCES
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/AuctionData.h
    src/AuctionData.cpp
    src/AuctionItem.h
    src/ResultExporter.h
    src/ResultExporter.cpp
    third_party/CJsonObject/cJSON.h
    third_party/CJsonObject/cJSON.c
    third_party/CJsonObject/CJsonObject.hpp
    third_party/CJsonObject/CJsonObject.cpp
)

add_executable(AuctionRecorder ${PROJECT_SOURCES})

target_include_directories(AuctionRecorder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/CJsonObject
)

target_link_libraries(AuctionRecorder PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# 源码是 UTF-8（无 BOM），MSVC 必须显式告诉编译器，否则中文字面量会乱码
if(MSVC)
    target_compile_options(AuctionRecorder PRIVATE /utf-8)
endif()

set_target_properties(AuctionRecorder PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE FALSE
)

# 把示例数据复制到可执行文件旁边，满足"① 可执行文件同目录"这一查找顺序
add_custom_command(TARGET AuctionRecorder POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AuctionRecorder>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/data/auction_data.json"
            "$<TARGET_FILE_DIR:AuctionRecorder>/data/auction_data.json"
    COMMENT "复制 auction_data.json 到构建输出目录"
)
