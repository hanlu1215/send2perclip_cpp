// 监听全局 Ctrl+V / Esc（对应 Python 版的 keyboard.add_hotkey + PasteWatcher）。
//
// 实现要点：
//   * 用 WH_KEYBOARD_LL 低级键盘钩子：它在按键送到目标程序之前就被调用，
//     所以目标程序（Excel、浏览器等）不需要是本程序，焦点在谁身上都能收到。
//   * 用「计数器」而不是「布尔标志」记录 Ctrl+V：布尔标志在重置的那一刻如果正好来了
//     Ctrl+V，这次按键就被吞掉了；计数器不会丢事件。
//   * 低级钩子的回调是在本线程调用 PeekMessage / GetMessage 时投递进来的，
//     所以等待期间必须不断泵消息（pumpMessages），否则收不到按键。
#pragma once

#include <windows.h>

#include <string>

class PasteWatcher {
public:
    PasteWatcher() = default;
    ~PasteWatcher();

    PasteWatcher(const PasteWatcher&) = delete;
    PasteWatcher& operator=(const PasteWatcher&) = delete;

    // 在当前线程安装键盘钩子；失败时把原因写进 *error（可为 nullptr）
    bool install(std::string* error);
    void uninstall();

    // 已经按下 Ctrl+V 的次数
    long long count() const { return count_; }
    // 用户是否按了 Esc
    bool exited() const { return exited_; }

    // 阻塞等待 Ctrl+V 次数达到 target；返回 false 表示用户按了 Esc。
    // 等待期间会泵消息，保证钩子回调能正常投递。
    bool waitForNext(long long target);

    // 等用户松开 Ctrl / V，避免长按的自动重复被算成新的粘贴
    void waitKeyRelease();

    // 处理挂起的消息（低级键盘钩子依赖它）
    void pumpMessages();

private:
    static LRESULT CALLBACK hookProc(int code, WPARAM wParam, LPARAM lParam);

    void registerPaste();
    bool anyKeyDown() const { return ctrl_left_ || ctrl_right_ || v_down_; }

    HHOOK hook_ = nullptr;
    long long count_ = 0;
    bool exited_ = false;
    bool ctrl_left_ = false;
    bool ctrl_right_ = false;
    bool v_down_ = false;
    unsigned long long last_paste_tick_ = 0;

    static PasteWatcher* instance_;
};
