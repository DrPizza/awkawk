//  Copyright (C) 2002 Peter Bright
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

// Description

#pragma once

#ifndef DEBUGSTRINGSTREAM__HPP
#define DEBUGSTRINGSTREAM__HPP

#include "locking_stream.hpp"
#include <sstream>
#include "../loki/singleton.h"

namespace utility
{

namespace
{
	extern "C"
	{
		void __stdcall OutputDebugStringA(const char* str);
		void __stdcall OutputDebugStringW(const wchar_t* str);
	}
	inline void outputDebugString_(const char* str) { OutputDebugStringA(str); }
	inline void outputDebugString_(const wchar_t* str) { OutputDebugStringW(str); }
}

template<typename Elem, typename Tr = std::char_traits<Elem>, typename Alloc = std::allocator<Elem> >
struct basic_debugstringbuf : std::basic_stringbuf<Elem, Tr, Alloc>
{
	typedef std::basic_string<Elem, Tr, Alloc> string_type;
	typedef basic_debugstringbuf<Elem, Tr, Alloc> my_type;
	typedef std::basic_stringbuf<Elem, Tr, Alloc> base_type;
	basic_debugstringbuf() : base_type(std::ios_base::out) {}
	virtual ~basic_debugstringbuf()
	{
		sync();
	}
protected:
	int sync(void)
	{
		outputDebugString_(this->str().c_str());
		this->str(string_type());
		return 0;
	}
private:
	basic_debugstringbuf(const basic_debugstringbuf&);
	my_type& operator=(const my_type&);
};

template<class Elem, class Tr = std::char_traits<Elem>, class Alloc = std::allocator<Elem> >
struct basic_debugostringstream : std::basic_ostream<Elem, Tr>
{
	typedef basic_debugstringbuf<Elem, Tr, Alloc> buffer_type;
	typedef std::basic_ostream<Elem, Tr> base_type;
	typedef basic_debugostringstream<Elem, Tr, Alloc> my_type;

	basic_debugostringstream() : base_type(new buffer_type()) {}
	virtual ~basic_debugostringstream()
	{
		delete this->rdbuf();
	}
private:
	basic_debugostringstream(const basic_debugostringstream&);
	my_type& operator=(const my_type&);
};

typedef basic_debugostringstream<char> dstream;
typedef basic_debugostringstream<wchar_t> wdstream;

namespace
{
	struct DebugStreamsImpl
	{
		utility::dstream dout;
		utility::wdstream wdout;
		utility::locking_ostream locked_dout;
		utility::wlocking_ostream locked_wdout;

		DebugStreamsImpl() : locked_dout(dout), locked_wdout(wdout)
		{
		}
	};
}

// notice that I rely on the CRT to perform initialization of statics in a safe manner.  Using a thread-safe policy crashes during static initialization.
typedef Loki::SingletonHolder<utility::DebugStreamsImpl, Loki::CreateUsingNew, Loki::DefaultLifetime, Loki::SingleThreaded> DebugStreams;

extern dstream& dout;
extern wdstream& wdout;

extern locking_ostream& locked_dout;
extern wlocking_ostream& locked_wdout;

} // namespace utility

#endif // !DEBUGSTRINGSTREAM__H
