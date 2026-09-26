#include "text.h"

#include <windows.h>

namespace text {

std::wstring widen(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    const int chars = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (chars <= 0) return std::wstring();
    std::wstring result(static_cast<size_t>(chars), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), result.data(), chars);
    return result;
}

std::string narrow(const std::wstring& wide) {
    if (wide.empty()) return std::string();
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return std::string();
    std::string result(static_cast<size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                        result.data(), bytes, nullptr, nullptr);
    return result;
}

}  // namespace text
