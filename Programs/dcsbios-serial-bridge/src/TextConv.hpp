#pragma once
/**
 * @file TextConv.hpp
 * @brief UTF-8 ⇄ UTF-16 conversion helpers for Win32 UI code.
 *
 * @details
 * Byte-by-byte widening (`std::wstring(s.begin(), s.end())`) garbles any
 * non-ASCII text. These helpers use the Win32 code-page APIs; invalid UTF-8
 * input is replaced with U+FFFD rather than failing.
 *
 * @copyright Copyright 2016-2026 Hornet Link contributors.
 *            Licensed under the Apache License, Version 2.0.
 */

#include <string>
#include <windows.h>

namespace dcsbios {

/// Convert UTF-8 text to UTF-16. Returns an empty string on failure.
inline std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (n <= 0) return {};
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), out.data(), n);
    return out;
}

/// Convert UTF-16 text to UTF-8. Returns an empty string on failure.
inline std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                                nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string out(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                        out.data(), n, nullptr, nullptr);
    return out;
}

} // namespace dcsbios
