// UTF-8 <-> UTF-16 转换工具。
//
// 程序内部统一用 UTF-8 的 std::string 处理文本（解析 CSV、打印日志），
// 只有在读写剪贴板时才转成 Windows 剪贴板使用的 UTF-16（std::wstring）。
#pragma once

#include <string>

namespace text {

// UTF-8 -> UTF-16（宽字符）
std::wstring widen(const std::string& utf8);

// UTF-16（宽字符）-> UTF-8
std::string narrow(const std::wstring& wide);

}  // namespace text
