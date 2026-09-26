#include "paste_watcher.h"

#include <cstdio>

#include "config.h"

PasteWatcher* PasteWatcher::instance_ = nullptr;

PasteWatcher::~PasteWatcher() { uninstall(); }

bool PasteWatcher::install(std::string* error) {
    if (hook_) return true;

    instance_ = this;
    hook_ = SetWindowsHookExW(WH_KEYBOARD_LL, &PasteWatcher::hookProc, GetModuleHandleW(nullptr), 0);
    if (!hook_) {
        const DWORD err = GetLastError();
        instance_ = nullptr;
        if (error) {
            char buf[128];
            std::snprintf(buf, sizeof(buf), "SetWindowsHookEx 失败，GetLastError=%lu", err);
            *error = buf;
        }
        return false;
    }
    return true;
}

void PasteWatcher::uninstall() {
    if (!hook_) return;
    UnhookWindowsHookEx(hook_);
    hook_ = nullptr;
    if (instance_ == this) instance_ = nullptr;
}

void PasteWatcher::pumpMessages() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            exited_ = true;
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

bool PasteWatcher::waitForNext(long long target) {
    for (;;) {
        pumpMessages();
        if (exited_) return false;
        if (count_ >= target) return true;
        Sleep(cfg::kPollIntervalMs);
    }
}

void PasteWatcher::waitKeyRelease() {
    while (anyKeyDown()) {
        pumpMessages();
        Sleep(10);
    }
    pumpMessages();
}

void PasteWatcher::registerPaste() {
    const unsigned long long now = GetTickCount64();
    if (now - last_paste_tick_ < static_cast<unsigned long long>(cfg::kDebounceMs)) {
        return;  // 长按产生的自动重复，忽略
    }
    last_paste_tick_ = now;
    ++count_;
}

LRESULT CALLBACK PasteWatcher::hookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && instance_ != nullptr) {
        const auto* kb = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        const bool key_down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        const bool key_up = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        PasteWatcher& self = *instance_;

        switch (kb->vkCode) {
            case VK_LCONTROL:
            case VK_CONTROL:
                if (key_down) self.ctrl_left_ = true;
                else if (key_up) self.ctrl_left_ = false;
                break;
            case VK_RCONTROL:
                if (key_down) self.ctrl_right_ = true;
                else if (key_up) self.ctrl_right_ = false;
                break;
            case 'V':
                if (key_down) {
                    self.v_down_ = true;
                    if (self.ctrl_left_ || self.ctrl_right_) self.registerPaste();
                } else if (key_up) {
                    self.v_down_ = false;
                }
                break;
            case VK_ESCAPE:
                if (key_down) self.exited_ = true;
                break;
            default:
                break;
        }
    }
    // 原样放行，不影响目标程序自己的按键处理
    return CallNextHookEx(nullptr, code, wParam, lParam);
}
