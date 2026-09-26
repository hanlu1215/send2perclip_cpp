// send2perclip —— 按 Ctrl+V 依次把剪贴板里的多行内容粘到目标程序里。
// 这是 send2perclip.py 的 C++（Win32）实现，行为与原脚本保持一致。
//
// 关于“偶发跳行”的说明（重要）：
//     低级键盘钩子在按键事件**送到目标程序之前**就被调用。如果一检测到 Ctrl+V 就立刻把下一行
//     写进剪贴板，而目标程序（Excel、浏览器输入框等）是在它自己的消息循环里处理按键后才去读
//     剪贴板的，那么当目标程序响应稍慢（窗口重绘、被遮挡、输入法、网页控件等）时，它读到的已经
//     是**下一行**了 —— 于是第 N 行被跳过、显示成第 N+1 行；而程序这时才开始等第 N+1 行的粘贴，
//     目标程序这次读得快，就把第 N+1 行又粘了一遍。这就是“跳过第 5 个、然后又出现第 6 个”的原因。
//
//     修复办法：检测到 Ctrl+V 后，先等用户松开按键，再额外等一小段时间（让目标程序把剪贴板读
//     完），才写入下一行。如果目标程序很慢，把 config.h 里的 kPasteSettleDelayMs 调大即可。

#include <windows.h>

#include <io.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "clipboard.h"
#include "config.h"
#include "csv.h"
#include "paste_watcher.h"
#include "text.h"

namespace {

// 提示音：与 Python 版的 print('\a') 一致 —— 输出到终端时写 BEL，
// 输出被重定向（控制台听不到 BEL）时用 Beep 兜底。
void beep() {
    if (_isatty(_fileno(stdout))) {
        std::fputc('\a', stdout);
        std::fflush(stdout);
    } else {
        Beep(1000, 150);
    }
}

std::string joinRow(const csv::Row& row) {
    std::string out;
    for (size_t i = 0; i < row.size(); ++i) {
        if (i != 0) out += ", ";
        out += row[i];
    }
    return out;
}

}  // namespace

int main() {
    // 控制台按 UTF-8 输出；源码里的中文字面量按 UTF-8 编译（见 CMakeLists.txt 的 /utf-8）
    SetConsoleOutputCP(CP_UTF8);

    std::vector<csv::Row> data = csv::parseRows(text::narrow(clipboard::readText()));
    data.erase(std::remove_if(data.begin(), data.end(), csv::isBlankRow), data.end());

    if (data.empty()) {
        std::printf("剪贴板里没有可用数据：请先复制要依次粘贴的内容，再运行本程序。\n");
        return 0;
    }

    PasteWatcher watcher;
    std::string error;
    if (!watcher.install(&error)) {
        std::printf("无法安装键盘钩子（%s）。\n", error.c_str());
        return 1;
    }

    std::printf("共 %zu 行数据。按 Ctrl+V 依次粘贴，按 Esc 退出。\n", data.size());
    std::fflush(stdout);

    for (size_t index = 1; index <= data.size(); ++index) {
        const long long done_count = watcher.count();  // 本次粘贴前的按键计数
        const std::string row_data = joinRow(data[index - 1]);

        if (!clipboard::writeText(text::widen(row_data), cfg::kClipboardRetries,
                                  cfg::kClipboardRetryDelayMs)) {
            std::printf("[!] 无法写入剪贴板，已跳过: %s\n", row_data.c_str());
            continue;
        }
        std::printf("[%zu/%zu] Copied: %s\n", index, data.size(), row_data.c_str());
        std::fflush(stdout);

        // 等待用户按下 Ctrl+V（次数要比上一次多 1）或 Esc
        if (!watcher.waitForNext(done_count + 1)) break;

        // ★关键修复：先等按键松开，再给目标程序留出读取剪贴板的时间，
        //   否则目标程序可能在下一条写入后才读到剪贴板 —— 表现为“跳过一行”。
        watcher.waitKeyRelease();
        Sleep(cfg::kPasteSettleDelayMs);

        // 每粘贴 kBeepEvery 条，发出提示音一次
        if (index % cfg::kBeepEvery == 0) beep();
    }

    watcher.uninstall();
    std::printf("Script finished.\n");
    return 0;
}
