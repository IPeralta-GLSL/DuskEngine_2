#pragma once

#ifndef _WIN32

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cwchar>
#include <cmath>
#include <climits>

#ifndef sprintf_s
#define sprintf_s(buf, size, fmt, ...) snprintf(buf, size, fmt, ##__VA_ARGS__)
#endif

#ifndef strcpy_s
inline int strcpy_s(char* dest, size_t destsz, const char* src)
{
    if (!dest || !src) return -1;
    strncpy(dest, src, destsz - 1);
    dest[destsz - 1] = '\0';
    return 0;
}
#endif

#ifndef wcscpy_s
inline int wcscpy_s(wchar_t* dest, const wchar_t* src)
{
    if (!dest || !src) return -1;
    wcscpy(dest, src);
    return 0;
}
inline int wcscpy_s(wchar_t* dest, size_t destsz, const wchar_t* src)
{
    if (!dest || !src) return -1;
    wcsncpy(dest, src, destsz - 1);
    dest[destsz - 1] = L'\0';
    return 0;
}
#endif

#ifndef memcpy_s
#define memcpy_s(dest, destsz, src, count) memcpy(dest, src, count)
#endif

#ifndef _countof
#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef wcstombs_s
inline int wcstombs_s(size_t* pReturnValue, char* mbstr, size_t sizeInBytes, const wchar_t* wcstr, size_t count)
{
    if (!mbstr || !wcstr) return -1;
    size_t converted = wcstombs(mbstr, wcstr, (count < sizeInBytes) ? count : sizeInBytes - 1);
    if (converted == static_cast<size_t>(-1))
    {
        mbstr[0] = '\0';
        if (pReturnValue) *pReturnValue = 0;
        return -1;
    }
    if (converted < sizeInBytes) mbstr[converted] = '\0';
    else mbstr[sizeInBytes - 1] = '\0';
    if (pReturnValue) *pReturnValue = converted;
    return 0;
}
#endif

#ifndef FFX_UNUSED
#define FFX_UNUSED(x) (void)(x)
#endif

#endif
