// Copyright 2013 Dolphin Emulator Project / 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <boost/range/algorithm/transform.hpp>
#include "common/common_paths.h"
#include "common/logging/log.h"
#include "common/string_util.h"

#ifdef _WIN32
#include <codecvt>
#include <windows.h>
#include "common/common_funcs.h"
#else
#include <iconv.h>
#endif

namespace Common {

/// Make a string lowercase
std::string ToLower(std::string str) {
    boost::transform(str, str.begin(), ::tolower);
    return str;
}

/// Make a string uppercase
std::string ToUpper(std::string str) {
    boost::transform(str, str.begin(), ::toupper);
    return str;
}

// For Debugging. Read out an u8 array.
std::string ArrayToString(const u8* data, size_t size, int line_len, bool spaces) {
    std::ostringstream oss;
    oss << std::setfill('0') << std::hex;

    for (int line = 0; size; ++data, --size) {
        oss << std::setw(2) << (int)*data;

        if (line_len == ++line) {
            oss << '\n';
            line = 0;
        } else if (spaces)
            oss << ' ';
    }

    return oss.str();
}

std::string StringFromBuffer(const std::vector<u8>& data) {
    return std::string(data.begin(), std::find(data.begin(), data.end(), '\0'));
}

// Turns "  hej " into "hej". Also handles tabs.
std::string StripSpaces(const std::string& str) {
    const size_t s = str.find_first_not_of(" \t\r\n");

    if (str.npos != s)
        return str.substr(s, str.find_last_not_of(" \t\r\n") - s + 1);
    else
        return "";
}

// "\"hello\"" is turned to "hello"
// This one assumes that the string has already been space stripped in both
// ends, as done by StripSpaces above, for example.
std::string StripQuotes(const std::string& s) {
    if (s.size() && '\"' == s[0] && '\"' == *s.rbegin())
        return s.substr(1, s.size() - 2);
    else
        return s;
}

bool TryParse(const std::string& str, u32* const output) {
    char* endptr = nullptr;

    // Reset errno to a value other than ERANGE
    errno = 0;

    unsigned long value = strtoul(str.c_str(), &endptr, 0);

    if (!endptr || *endptr)
        return false;

    if (errno == ERANGE)
        return false;

#if ULONG_MAX > UINT_MAX
    if (value >= 0x100000000ull && value <= 0xFFFFFFFF00000000ull)
        return false;
#endif

    *output = static_cast<u32>(value);
    return true;
}

bool TryParse(const std::string& str, bool* const output) {
    if ("1" == str || "true" == ToLower(str))
        *output = true;
    else if ("0" == str || "false" == ToLower(str))
        *output = false;
    else
        return false;

    return true;
}

std::string StringFromBool(bool value) {
    return value ? "True" : "False";
}

bool SplitPath(const std::string& full_path, std::string* _pPath, std::string* _pFilename,
               std::string* _pExtension) {
    if (full_path.empty())
        return false;

    size_t dir_end = full_path.find_last_of("/"
// windows needs the : included for something like just "C:" to be considered a directory
#ifdef _WIN32
                                            "\\:"
#endif
    );
    if (std::string::npos == dir_end)
        dir_end = 0;
    else
        dir_end += 1;

    size_t fname_end = full_path.rfind('.');
    if (fname_end < dir_end || std::string::npos == fname_end)
        fname_end = full_path.size();

    if (_pPath)
        *_pPath = full_path.substr(0, dir_end);

    if (_pFilename)
        *_pFilename = full_path.substr(dir_end, fname_end - dir_end);

    if (_pExtension)
        *_pExtension = full_path.substr(fname_end);

    return true;
}

void BuildCompleteFilename(std::string& _CompleteFilename, const std::string& _Path,
                           const std::string& _Filename) {
    _CompleteFilename = _Path;

    // check for seperator
    if (DIR_SEP_CHR != *_CompleteFilename.rbegin())
        _CompleteFilename += DIR_SEP_CHR;

    // add the filename
    _CompleteFilename += _Filename;
}

void SplitString(const std::string& str, const char delim, std::vector<std::string>& output) {
    std::istringstream iss(str);
    output.resize(1);

    while (std::getline(iss, *output.rbegin(), delim)) {
        output.emplace_back();
    }

    output.pop_back();
}

std::string TabsToSpaces(int tab_size, std::string in) {
    size_t i = 0;

    while ((i = in.find('\t')) != std::string::npos) {
        in.replace(i, 1, tab_size, ' ');
    }

    return in;
}

std::string ReplaceAll(std::string result, const std::string& src, const std::string& dest) {
    size_t pos = 0;

    if (src == dest)
        return result;

    while ((pos = result.find(src, pos)) != std::string::npos) {
        result.replace(pos, src.size(), dest);
        pos += dest.length();
    }

    return result;
}

#ifdef _WIN32

std::string UTF16ToUTF8(const std::u16string& input) {
#if _MSC_VER >= 1900
    // Workaround for missing char16_t/char32_t instantiations in MSVC2015
    std::wstring_convert<std::codecvt_utf8_utf16<__int16>, __int16> convert;
    std::basic_string<__int16> tmp_buffer(input.cbegin(), input.cend());
    return convert.to_bytes(tmp_buffer);
#else
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return convert.to_bytes(input);
#endif
}

std::u16string UTF8ToUTF16(const std::string& input) {
#if _MSC_VER >= 1900
    // Workaround for missing char16_t/char32_t instantiations in MSVC2015
    std::wstring_convert<std::codecvt_utf8_utf16<__int16>, __int16> convert;
    auto tmp_buffer = convert.from_bytes(input);
    return std::u16string(tmp_buffer.cbegin(), tmp_buffer.cend());
#else
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return convert.from_bytes(input);
#endif
}

static std::wstring CPToUTF16(u32 code_page, const std::string& input) {
    const auto size =
        MultiByteToWideChar(code_page, 0, input.data(), static_cast<int>(input.size()), nullptr, 0);

    if (size == 0) {
        return L"";
    }

    std::wstring output(size, L'\0');

    if (size != MultiByteToWideChar(code_page, 0, input.data(), static_cast<int>(input.size()),
                                    &output[0], static_cast<int>(output.size()))) {
        output.clear();
    }

    return output;
}

std::string UTF16ToUTF8(const std::wstring& input) {
    const auto size = WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (size == 0) {
        return "";
    }

    std::string output(size, '\0');

    if (size != WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()),
                                    &output[0], static_cast<int>(output.size()), nullptr,
                                    nullptr)) {
        output.clear();
    }

    return output;
}

std::wstring UTF8ToUTF16W(const std::string& input) {
    return CPToUTF16(CP_UTF8, input);
}

std::string SHIFTJISToUTF8(const std::string& input) {
    return UTF16ToUTF8(CPToUTF16(932, input));
}

std::string CP1252ToUTF8(const std::string& input) {
    return UTF16ToUTF8(CPToUTF16(1252, input));
}

#else

template <typename T>
static std::string CodeToUTF8(const char* fromcode, const std::basic_string<T>& input) {
    iconv_t const conv_desc = iconv_open("UTF-8", fromcode);
    if ((iconv_t)(-1) == conv_desc) {
        LOG_ERROR(Common, "Iconv initialization failure [{}]: {}", fromcode, strerror(errno));
        iconv_close(conv_desc);
        return {};
    }

    const size_t in_bytes = sizeof(T) * input.size();
    // Multiply by 4, which is the max number of bytes to encode a codepoint
    const size_t out_buffer_size = 4 * in_bytes;

    std::string out_buffer(out_buffer_size, '\0');

    auto src_buffer = &input[0];
    size_t src_bytes = in_bytes;
    auto dst_buffer = &out_buffer[0];
    size_t dst_bytes = out_buffer.size();

    while (0 != src_bytes) {
        size_t const iconv_result =
            iconv(conv_desc, (char**)(&src_buffer), &src_bytes, &dst_buffer, &dst_bytes);

        if (static_cast<size_t>(-1) == iconv_result) {
            if (EILSEQ == errno || EINVAL == errno) {
                // Try to skip the bad character
                if (0 != src_bytes) {
                    --src_bytes;
                    ++src_buffer;
                }
            } else {
                LOG_ERROR(Common, "iconv failure [{}]: {}", fromcode, strerror(errno));
                break;
            }
        }
    }

    std::string result;
    out_buffer.resize(out_buffer_size - dst_bytes);
    out_buffer.swap(result);

    iconv_close(conv_desc);

    return result;
}

std::u16string UTF8ToUTF16(const std::string& input) {
    iconv_t const conv_desc = iconv_open("UTF-16LE", "UTF-8");
    if ((iconv_t)(-1) == conv_desc) {
        LOG_ERROR(Common, "Iconv initialization failure [UTF-8]: {}", strerror(errno));
        iconv_close(conv_desc);
        return {};
    }

    const size_t in_bytes = sizeof(char) * input.size();
    // Multiply by 4, which is the max number of bytes to encode a codepoint
    const size_t out_buffer_size = 4 * sizeof(char16_t) * in_bytes;

    std::u16string out_buffer(out_buffer_size, char16_t{});

    char* src_buffer = const_cast<char*>(&input[0]);
    size_t src_bytes = in_bytes;
    char* dst_buffer = (char*)(&out_buffer[0]);
    size_t dst_bytes = out_buffer.size();

    while (0 != src_bytes) {
        size_t const iconv_result =
            iconv(conv_desc, &src_buffer, &src_bytes, &dst_buffer, &dst_bytes);

        if (static_cast<size_t>(-1) == iconv_result) {
            if (EILSEQ == errno || EINVAL == errno) {
                // Try to skip the bad character
                if (0 != src_bytes) {
                    --src_bytes;
                    ++src_buffer;
                }
            } else {
                LOG_ERROR(Common, "iconv failure [UTF-8]: {}", strerror(errno));
                break;
            }
        }
    }

    std::u16string result;
    out_buffer.resize(out_buffer_size - dst_bytes);
    out_buffer.swap(result);

    iconv_close(conv_desc);

    return result;
}

std::string UTF16ToUTF8(const std::u16string& input) {
    return CodeToUTF8("UTF-16LE", input);
}

std::string CP1252ToUTF8(const std::string& input) {
    // return CodeToUTF8("CP1252//TRANSLIT", input);
    // return CodeToUTF8("CP1252//IGNORE", input);
    return CodeToUTF8("CP1252", input);
}

std::string SHIFTJISToUTF8(const std::string& input) {
    // return CodeToUTF8("CP932", input);
    return CodeToUTF8("SJIS", input);
}

#endif

std::string StringFromFixedZeroTerminatedBuffer(const char* buffer, size_t max_len) {
    size_t len = 0;
    while (len < max_len && buffer[len] != '\0')
        ++len;

    return std::string(buffer, len);
}

const char* TrimSourcePath(const char* path, const char* root) {
    const char* p = path;

    while (*p != '\0') {
        const char* next_slash = p;
        while (*next_slash != '\0' && *next_slash != '/' && *next_slash != '\\') {
            ++next_slash;
        }

        bool is_src = Common::ComparePartialString(p, next_slash, root);
        p = next_slash;

        if (*p != '\0') {
            ++p;
        }
        if (is_src) {
            path = p;
        }
    }
    return path;
}

} // namespace Common
