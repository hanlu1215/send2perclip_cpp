// 剪贴板读写（对应 Python 版的 pyperclip.paste / pyperclip.copy）。
#pragma once

#include <string>

namespace clipboard {

// 读取剪贴板里的文本（优先 CF_UNICODETEXT，退而用 CF_TEXT）。
// 读不到时返回空串。
std::wstring readText();

// 把文本写进剪贴板。别的程序占用剪贴板时会重试 retries 次，每次间隔 retry_delay_ms 毫秒。
// 成功返回 true。
bool writeText(const std::wstring& text, int retries, int retry_delay_ms);

}  // namespace clipboard
