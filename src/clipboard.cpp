#include "clipboard.h"

#include <windows.h>

#include <cstring>

#include "config.h"

namespace clipboard {
namespace {

// 剪贴板同一时刻只允许一个进程打开，失败就等一会儿再试
bool openClipboardWithRetry(int retries, int retry_delay_ms) {
    for (int attempt = 0; attempt < retries; ++attempt) {
        if (OpenClipboard(nullptr)) return true;
        Sleep(static_cast<DWORD>(retry_delay_ms));
    }
    return false;
}

}  // namespace

std::wstring readText() {
    std::wstring result;
    if (!openClipboardWithRetry(cfg::kClipboardRetries, cfg::kClipboardRetryDelayMs)) return result;

    if (HANDLE handle = GetClipboardData(CF_UNICODETEXT)) {
        if (const auto* p = static_cast<const wchar_t*>(GlobalLock(handle))) {
            result.assign(p);
            GlobalUnlock(handle);
        }
    } else if (HANDLE handle = GetClipboardData(CF_TEXT)) {
        // 只有 ANSI 文本时，按系统代码页转换过来
        if (const auto* p = static_cast<const char*>(GlobalLock(handle))) {
            const int chars = MultiByteToWideChar(CP_ACP, 0, p, -1, nullptr, 0);
            if (chars > 1) {
                result.resize(static_cast<size_t>(chars - 1));
                MultiByteToWideChar(CP_ACP, 0, p, -1, result.data(), chars);
            }
            GlobalUnlock(handle);
        }
    }

    CloseClipboard();
    return result;
}

bool writeText(const std::wstring& text, int retries, int retry_delay_ms) {
    for (int attempt = 0; attempt < retries; ++attempt) {
        if (openClipboardWithRetry(1, 0)) {
            bool ok = false;
            if (EmptyClipboard()) {
                const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
                if (HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes)) {
                    if (void* dst = GlobalLock(mem)) {
                        std::memcpy(dst, text.c_str(), bytes);
                        GlobalUnlock(mem);
                        // 成功后所有权归剪贴板，不能再 GlobalFree
                        ok = SetClipboardData(CF_UNICODETEXT, mem) != nullptr;
                    }
                    if (!ok) GlobalFree(mem);
                }
            }
            CloseClipboard();
            if (ok) return true;
        }
        Sleep(static_cast<DWORD>(retry_delay_ms));
    }
    return false;
}

}  // namespace clipboard
