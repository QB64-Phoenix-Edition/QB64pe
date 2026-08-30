#pragma once

#include <codecvt>
#include <cstdint>
#include <locale>
#include <string>
#include <vector>

// Forward declaration from libqb.cpp
extern uint16_t codepage437_to_unicode16[];

/// @brief A class that manages conversions between various encodings (UTF-8, UTF-16, UTF-32, CP437).
/// Note: This class uses the deprecated codecvt library from C++17.
/// We will need to replace this with a better implementation in the future when we adopt C++26 or later.
class Unicode {
    static constexpr uint32_t MAX_UNICODE_CODEPOINT = 0x10FFFFu;

    // Internal reusable UTF-32 buffer
    std::u32string string;
    // Reused converters
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convUTF8;
    std::wstring_convert<std::codecvt_utf16<char32_t, MAX_UNICODE_CODEPOINT,
                                            static_cast<std::codecvt_mode>(std::codecvt_mode::consume_header | std::codecvt_mode::little_endian)>,
                         char32_t>
        convUTF16LEBOM;
    std::wstring_convert<std::codecvt_utf16<char32_t, MAX_UNICODE_CODEPOINT, std::codecvt_mode::consume_header>, char32_t> convUTF16BEBOM;
    std::wstring_convert<std::codecvt_utf16<char32_t, MAX_UNICODE_CODEPOINT, std::codecvt_mode::little_endian>, char32_t> convUTF16LENoBOM;

  public:
    Unicode() = default;
    Unicode(const Unicode &) = delete;
    Unicode &operator=(const Unicode &) = delete;
    Unicode(Unicode &&) noexcept = default;
    Unicode &operator=(Unicode &&) noexcept = default;

    /// @brief Converts a code page 437 byte string to UTF-32.
    [[nodiscard]] size_t ConvertCP437(const uint8_t *str, size_t len) noexcept {
        string.resize(len);
        for (size_t i = 0; i < len; i++) {
            string[i] = codepage437_to_unicode16[str[i]];
        }
        return string.size();
    }

    /// @brief Converts UTF-8 byte string to UTF-32.
    [[nodiscard]] size_t ConvertUTF8(const uint8_t *str, size_t len) {
        try {
            string = convUTF8.from_bytes(reinterpret_cast<const char *>(str), reinterpret_cast<const char *>(str + len));
        } catch (...) {
            string.clear();
        }
        return string.size();
    }

    /// @brief Converts UTF-16 (LE/BE) byte string to UTF-32.
    [[nodiscard]] size_t ConvertUTF16(const uint8_t *str, size_t len) {
        try {
            bool hasLEBOM, hasBEBOM;
            if (len >= 2) {
                hasLEBOM = (str[0] == 0xFF && str[1] == 0xFE);
                hasBEBOM = (str[0] == 0xFE && str[1] == 0xFF);
            } else {
                hasLEBOM = false;
                hasBEBOM = false;
            }

            if (hasLEBOM) {
                string = convUTF16LEBOM.from_bytes(reinterpret_cast<const char *>(str), reinterpret_cast<const char *>(str + len));
            } else if (hasBEBOM) {
                string = convUTF16BEBOM.from_bytes(reinterpret_cast<const char *>(str), reinterpret_cast<const char *>(str + len));
            } else {
                string = convUTF16LENoBOM.from_bytes(reinterpret_cast<const char *>(str), reinterpret_cast<const char *>(str + len));
            }
        } catch (...) {
            string.clear();
        }
        return string.size();
    }

    /// @brief Returns the converted UTF-32 string.
    [[nodiscard]] const std::u32string &GetString() const noexcept {
        return string;
    }

    /// @brief Sets the internal UTF-32 string directly.
    void SetString(const std::u32string &str) {
        string = str;
    }

    /// @brief Sets the internal UTF-32 string to a single codepoint.
    void SetCodepoint(char32_t cp) {
        string.assign(1, cp);
    }

    /// @brief Converts the current UTF-32 string to UTF-8
    [[nodiscard]] std::string ToUTF8() {
        try {
            return convUTF8.to_bytes(string);
        } catch (...) {
            return "";
        }
    }

    /// @brief Converts the current UTF-32 string to UTF-16 (Little Endian, no BOM)
    [[nodiscard]] std::string ToUTF16() {
        try {
            return convUTF16LENoBOM.to_bytes(string);
        } catch (...) {
            return "";
        }
    }

    /// @brief Converts the current UTF-32 string to CP437
    [[nodiscard]] std::string ToCP437() const noexcept {
        std::string out;
        out.reserve(string.size());
        for (char32_t cp : string) {
            bool found = false;
            for (int i = 0; i < 256; ++i) {
                if (codepage437_to_unicode16[i] == cp) {
                    out.push_back(static_cast<char>(i));
                    found = true;
                    break;
                }
            }
            if (!found)
                out.push_back('?'); // fallback for unmappable characters
        }
        return out;
    }
};
