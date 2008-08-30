//  Copyright (C) 2002-2006 Peter Bright
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//
//  Peter Bright <drpizza@quiscalusmexicanus.org>

#pragma once

#ifndef STRING__HPP
#define STRING__HPP

#include <cstring>

namespace utility
{

inline size_t string_length(const char* str)
{
	return std::strlen(str);
}
inline size_t string_length(const wchar_t* str)
{
	return std::wcslen(str);
}

inline size_t strlen(const char* str)
{
	return std::strlen(str);
}
inline size_t strlen(const wchar_t* str)
{
	return std::wcslen(str);
}

inline const char* strchr(const char* str, int val)
{
	return std::strchr(str, val);
}

inline const wchar_t* strchr(const wchar_t* str, wchar_t val)
{
	return std::wcschr(str, val);
}

inline int strncmp(const char* str1, const char* str2, size_t n)
{
	return std::strncmp(str1, str2, n);
}

inline int string_compare(const char* str1, const char* str2, size_t n)
{
	return std::strncmp(str1, str2, n);
}

inline int strncmp(const wchar_t* str1, const wchar_t* str2, size_t n)
{
	return std::wcsncmp(str1, str2, n);
}

inline int string_compare(const wchar_t* str1, const wchar_t* str2, size_t n)
{
	return std::wcsncmp(str1, str2, n);
}

}

#endif // #ifndef STRING_HPP