// 可调参数（与 send2perclip.py 顶部的常量一一对应）
#pragma once

namespace cfg {

// 检测到 Ctrl+V 后，等用户松开按键，再额外等待这么久（毫秒）才写入下一条。
// 目标程序（Excel、网页、输入法）比较慢时，可改成 400 甚至 500。
inline constexpr int kPasteSettleDelayMs = 250;

// 长按 Ctrl+V 的自动重复 / 极短时间内的重复触发，在这个窗口内只算一次粘贴
inline constexpr int kDebounceMs = 150;

// 主循环轮询间隔（毫秒），同时决定泵消息（投递键盘钩子回调）的频率
inline constexpr int kPollIntervalMs = 20;

// 每粘贴多少条响一次提示音
inline constexpr int kBeepEvery = 9;

// 目标程序正在占用剪贴板时的重试次数与重试间隔（毫秒）
inline constexpr int kClipboardRetries = 10;
inline constexpr int kClipboardRetryDelayMs = 50;

}  // namespace cfg
