# 人生拍卖会 · 拍卖结果记录（AuctionRecorder）

大班会课现场用的小工具。启动时从 JSON 读取拍品和参赛班级，逐件拍卖，每件限时若干秒；倒计时结束（或拍卖师手动提前结束）后录入成交价和买家班级，也可以标记无人竞拍。全部拍完后以表格展示结果，统计成交件数、流拍件数、总金额和各班拍得的件数，并导出 CSV 给 Excel。

- 语言：C++
- 界面：Qt Widgets（Qt 5.12+ / Qt 6，一套代码两边都能编）
- JSON：CJsonObject（作者 bwarliao），没有用 QJsonDocument
- 纯本地，不联网

## 目录结构

```
拍卖/
├── CMakeLists.txt              CMake 构建（优先用这个）
├── AuctionRecorder.pro         qmake 构建
├── README.md
├── data/
│   └── auction_data.json       示例数据
├── third_party/
│   └── CJsonObject/
│       ├── cJSON.h
│       ├── cJSON.c
│       ├── CJsonObject.hpp
│       └── CJsonObject.cpp
├── src/
│   ├── main.cpp                入口、高 DPI、中文字体选择
│   ├── MainWindow.h/.cpp       三个页面 + 倒计时 + 录入对话框
│   ├── AuctionData.h/.cpp      读 JSON、校验、暴露纯 Qt 容器
│   ├── AuctionItem.h           AuctionItem / AuctionRecord 结构体
│   └── ResultExporter.h/.cpp   CSV 导出
└── tools/
    └── test_data.cpp           可选，命令行验证 JSON 能否正常解析
```

CJsonObject 的四个文件原本放在根目录，现在按工程约定移到了 `third_party/CJsonObject/`，两个构建脚本都按这个路径写，不用再改。

## 环境要求

| 项目 | 要求 |
| --- | --- |
| 编译器 | 支持 C++11；用 Qt6 时需支持 C++17 |
| Qt | 5.12 及以上，或 6.x，必须有 Widgets 模块 |
| 构建工具 | CMake 3.16+，或 qmake |
| 其他依赖 | 无 |

Qt 官方安装器里勾选版本对应的 MSVC 或 MinGW 套件即可；Linux 发行版装 `qtbase5-dev` 或 `qt6-base-dev`。

## 编译

### Linux（gcc）

Qt5：

```bash
sudo apt install build-essential cmake qtbase5-dev
cd /path/to/拍卖
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/AuctionRecorder
```

Qt6 把 `qtbase5-dev` 换成 `qt6-base-dev`，其余命令一样。

系统里同时装了 Qt5 和 Qt6 时，CMake 优先找 Qt6。想强制用 Qt5：

```bash
cmake -S . -B build -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
```

qmake 方式：

```bash
sudo apt install qtbase5-dev qtbase5-dev-tools
mkdir -p build && cd build
qmake ../AuctionRecorder.pro
make -j"$(nproc)"
```

qmake 构建不会自动复制 data 目录，运行前手动把 `data/auction_data.json` 放到可执行文件旁边，或者在界面上点「选择数据文件…」。

### Windows

CMake + MSVC，在「x64 Native Tools Command Prompt for VS」里执行：

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH=C:/Qt/6.7.2/msvc2019_64
cmake --build build --config Release
build\Release\AuctionRecorder.exe
```

CMake + MinGW：

```bat
cmake -S . -B build -G "MinGW Makefiles" ^
      -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/mingw73_64
cmake --build build -j
```

`CMAKE_PREFIX_PATH` 指向 Qt 的安装子目录（例如 `C:/Qt/6.7.2/msvc2019_64`），不是 `bin`，也不是 `C:/Qt`。

MSVC 的 `/utf-8` 选项已经写进 CMakeLists.txt 和 .pro，别删。删了中文字面量会按本地代码页解析，界面全是乱码。

qmake 方式：

```bat
qmake AuctionRecorder.pro
nmake            :: MSVC
mingw32-make     :: MinGW
```

Windows 下 qmake 不复制数据文件，把 `data` 整个目录拷到 exe 旁边。

### macOS（clang）

```bash
brew install qt cmake
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build -j
./build/AuctionRecorder
```

默认不打成 `.app`（CMakeLists 里 `MACOSX_BUNDLE FALSE`），直接跑可执行文件就能用。想打包成 .app 就把那行改成 TRUE，程序会额外在 `Contents/Resources/` 里找数据文件。

### Qt Creator

打开 `CMakeLists.txt`，选好 Kit，直接点运行。不用额外配置。

## 运行与数据文件查找

启动时按顺序找 `auction_data.json`，第一个存在的就用：

1. 可执行文件同目录 `data/auction_data.json`
2. 可执行文件同目录 `auction_data.json`
3. macOS：`../Resources/auction_data.json`（打成 .app 时）
4. `QStandardPaths::AppDataLocation` 目录下
5. 都没有就停在准备页，提示用户点「选择数据文件…」

`AppDataLocation` 在三个平台的实际路径：

| 平台 | 路径 |
| --- | --- |
| Windows | `C:/Users/<用户名>/AppData/Roaming/AuctionRecorder/AuctionRecorder/` |
| macOS | `~/Library/Application Support/AuctionRecorder/AuctionRecorder/` |
| Linux | `~/.local/share/AuctionRecorder/AuctionRecorder/` |

程序只认这几个位置，不会去读源码目录，所以从 build 目录运行时记得让数据文件跟着可执行文件走。

## JSON 字段说明

```json
{
  "eventName": "人生拍卖会",
  "timeLimitSeconds": 30,
  "items": [
    { "id": 1, "name": "一张世界名校录取通知书", "description": "任何专业，随时入学", "startPrice": 100000 }
  ],
  "classes": [
    { "id": 1, "name": "高二(1)班" }
  ]
}
```

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `eventName` | string | 否 | 活动名称，缺省「人生拍卖会」 |
| `timeLimitSeconds` | int | 否 | 每件限时秒数，1~3600，缺省 30 |
| `items` | array | 是 | 至少一件；数量不固定 |
| `items[].id` | int | 否 | 缺省按数组顺序编号 |
| `items[].name` | string | 是 | 拍品名称 |
| `items[].description` | string | 否 | 拍品说明，界面里居中显示 |
| `items[].startPrice` | number | 是 | 起拍价，单位元，不能为负 |
| `classes` | array | 是 | 至少一个班级 |
| `classes[].id` | int | 否 | 保留字段，界面按顺序显示 |
| `classes[].name` | string | 是 | 班级名称，不能重复 |

金额统一按「元」显示，界面上带千位分隔符。

错误处理覆盖这些情况，全部中文提示：

- 文件不存在、不是文件、没有读权限、内容为空
- JSON 语法错误（括号逗号引号问题）
- 字段缺失或类型不对：一次性把所有问题列出来，例如「数据文件存在 2 处问题」
- 成交价低于起拍价、成交价不是数字
- 导出失败（路径不可写、写入中断）

文件编码 UTF-8，带不带 BOM 都行，程序会先剥掉 BOM 再解析。

## 使用流程

1. 准备页显示活动名称、拍品总数、班级数、每件限时，下面是拍品预览表（序号、名称、起拍价）。
2. 点「开始拍卖」进入拍卖页：第几件/共几件、拍品名、说明、起拍价、大号倒计时、进度条。最后 5 秒变红并闪烁，时间到会响一声提示音。
3. 倒计时结束或者点「提前结束计时」，弹出录入框：填成交价（≥ 起拍价），选买家班级；下拉框最后一项是「无人竞拍（流拍）」，选了它价格框自动禁用。
4. 确认后进入下一件。「上一件」可以回退修改，原来的值会填回输入框，改完继续往下走。
5. 全部录完进结果页：表格六列（序号、拍品名称、起拍价、成交价、买家班级、状态），下面两行统计，分别是成交/流拍件数、总成交金额，以及各班级拍得件数。
6. 「导出 CSV」选保存位置；「重新开始」清空结果回到准备页。

拍卖没结束时关窗口会先问一句，避免手滑把结果丢了。

## 导出 CSV

- 用 `QFileDialog::getSaveFileName`，各平台原生对话框
- UTF-8 带 BOM，Windows 版 Excel 双击打开中文正常
- 行尾 CRLF；字段里有逗号、引号、换行就整体加引号，内部引号翻倍
- 内容包括：活动名称、导出时间、明细表、统计块（成交件数、流拍件数、总成交金额、拍品总数）、各班级拍得件数

不想要统计块的话改 `ResultExporter::exportCsv` 里 `明细表` 之后那段即可。

## 跨平台注意事项

**编码**：源码统一 UTF-8 无 BOM。MSVC 必须加 `/utf-8`（已经加好）。用 VS Code 编辑时注意右下角别存成 GBK。

**路径**：所有路径都用 `QDir` / `QFileInfo` / `QStandardPaths` 拼接，没有硬编码 `\` 或 `/`。界面上显示路径时用 `QDir::toNativeSeparators` 转成当前平台的风格。

**字体**：`main.cpp` 里按 `Microsoft YaHei UI` → `PingFang SC` → `Hiragino Sans GB` → `Noto Sans CJK SC` → `Source Han Sans SC` → `WenQuanYi Micro Hei` → `SimHei` 的顺序，从 `QFontDatabase` 里挑第一个系统真装了的；一个都没找到就交给 Qt 自己回退，不会硬套 Windows 专有字体。

**高 DPI**：Qt5 在 `main()` 里开 `AA_EnableHighDpiScaling` 和 `AA_UseHighDpiPixmaps`；Qt6 默认已开，代码用版本宏隔开，不会重复设置。样式表里的字号写的是 px，Qt 会按缩放比换算。

**CSV BOM**：只有 Windows Excel 认这个，macOS / Linux 的表格软件会忽略，所以统一写。

## 常见问题

**cmake 报找不到 Qt**：`-DCMAKE_PREFIX_PATH` 要指向 Qt 版本的安装目录，比如 `C:/Qt/6.7.2/msvc2019_64`，不是 `bin` 子目录。

**Windows 编译后界面中文乱码**：确认用 MSVC 编译且带了 `/utf-8`，源文件编码是 UTF-8。

**启动后提示找不到数据文件**：程序不会去读源码目录。CMake 构建会自动把 `data/auction_data.json` 复制到输出目录；qmake 构建要手动复制，或者点「选择数据文件…」。

**导出后在 Excel 里乱码**：只认本程序导出的文件（带 BOM）。如果用记事本打开再另存为 ANSI，中文就废了。

**倒计时没声音**：`QApplication::beep()` 走系统默认提示音，个别 Linux 桌面没配声音主题就是静音，不影响功能。

**Qt6 下编译报某个 API 不存在**：代码是按 Qt5/Qt6 双兼容写的，CMakeLists 在 Qt6 下会自动切 C++17。如果还报错，把报错贴出来，多半是某个 5→6 的签名差异。

**点「上一件」改完会继续往下拍**：设计如此。回退只改这一件，改完顺着往后走，不会把后面的记录当成草稿。

**界面字号在投影上看不清**：所有字号集中在 `MainWindow::applyStyle()` 的样式表里，想整体放大改那几行 px 值即可。

## 关键实现思路

**数据加载**：`AuctionData` 只依赖 CJsonObject 和 Qt 容器，界面层拿到的就是 `QVector<AuctionItem>` 和 `QStringList`。读文件用 `QFile` 拿 `QByteArray`，剥掉 UTF-8 BOM 后交给 `CJsonObject::Parse`，然后逐字段校验。校验问题攒成一个列表一次性返回，用户改一遍就能全改完。任何一项不过就 `clear()`，不给界面留半截数据。

**倒计时**：`QTimer` 一秒一跳，剩余秒数存在成员里，进度条同步刷新。剩 5 秒时启动一个 400ms 的闪烁定时器，在深红和浅红之间来回切；归零就停表、保持红色、`beep()`，直接弹录入框。手动「提前结束计时」和自动超时走的是同一个函数，两条路径行为完全一致，不会出现手动结束不弹框的情况。

**结果录入**：`QDialog` + `QFormLayout`。价格框挂 `QRegularExpressionValidator` 只收数字，点确认时再判一次「不能为空、不能小于起拍价、必须是整数」，不合法就留在对话框里并选中文本方便重填。选「无人竞拍」时价格框禁用，记录 `sold=false`。点取消不算出错，停在当前件，按钮文字变成「录入结果」，不会自动跳下一件。

**导航**：一个下标 `m_index` 表示当前拍品，`m_records` 与 `items` 等长、一对一。上一件就是 `m_index--`，然后把原记录回填进对话框，改完接着往下走。

**导出**：`ResultExporter` 是静态类，输入拍品、记录、班级列表，输出一个 `QFile` 写入的字节流，界面只负责选路径和弹提示。导出的判定逻辑（成交/流拍、总额、各班件数）在结果页和导出里各写了一遍，两边口径一致。

## 验证情况

Linux（Debian 12，g++ 12.2，Qt 5.15）下用 g++ 手工编译过：全部源文件 `-Wall -Wextra` 零警告，链接通过。数据层实测过五种输入——正常文件、文件不存在、JSON 语法错误、字段缺失加类型错误、班级数组为空，都按预期输出中文提示和问题清单。验证程序就是 `tools/test_data.cpp`：

```bash
g++ -std=c++11 -fPIC $(pkg-config --cflags Qt5Core) -Isrc -Ithird_party/CJsonObject \
    tools/test_data.cpp src/AuctionData.cpp \
    third_party/CJsonObject/CJsonObject.cpp third_party/CJsonObject/cJSON.c \
    $(pkg-config --libs Qt5Core) -o test_data
./test_data data/auction_data.json
```

界面部分没做视觉验证（那台机器上没有完整图形环境），排版和字号建议本机跑一遍看一眼。Qt6 只做了代码层面的兼容处理，没有在 Qt6 环境里真编过，遇到报错贴出来就行。
