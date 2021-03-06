//  Copyright (C) 2002-2008 Peter Bright
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

#ifndef LOCKING_STREAM__HPP
#define LOCKING_STREAM__HPP

#include <iosfwd>
#include <typeinfo>
#include "locks.hpp"

namespace utility
{
template<typename E, typename T = std::char_traits<E> >
struct basic_locking_ostream
{
	std::shared_ptr<utility::critical_section> cs;
	std::basic_ostream<E, T>* s;

	basic_locking_ostream(std::basic_ostream<E, T>& os) : s(&os), cs(new utility::critical_section(std::string("basic_locking_ostream<>")))
	{
	}

	template<typename E2, typename T2>
	basic_locking_ostream(std::basic_ostream<E, T>& os, basic_locking_ostream<E2, T2>& other_stream) : s(&os), cs(other_stream.cs)
	{
	}

	struct basic_locking_ostream_helper
	{
		basic_locking_ostream<E, T>* ls;

		basic_locking_ostream_helper(basic_locking_ostream<E, T>* l) : ls(l)
		{
			ls->cs->enter();
		}
		basic_locking_ostream_helper(const basic_locking_ostream_helper& rhs) : ls(rhs.ls)
		{
			ls->cs->enter();
		}
		~basic_locking_ostream_helper()
		{
			ls->cs->leave();
		}

		template<typename TT>
		basic_locking_ostream_helper& operator<<(const TT& rhs)
		{
			*(ls->s) << rhs;
			return *this;
		}
		basic_locking_ostream_helper& operator<<(std::basic_ostream<E, T>& (*manip)(std::basic_ostream<E, T>&))
		{
			*(ls->s) << manip;
			return *this;
		}
	};

	template<typename TT>
	basic_locking_ostream_helper operator<<(const TT& rhs)
	{
		basic_locking_ostream_helper hlp(this);
		*s << rhs;
		return hlp;
	}
	basic_locking_ostream_helper operator<<(std::basic_ostream<E, T>& (*manip)(std::basic_ostream<E, T>&))
	{
		basic_locking_ostream_helper hlp(this);
		*s << manip;
		return hlp;
	}
};

typedef basic_locking_ostream<char> locking_ostream;
typedef basic_locking_ostream<wchar_t> wlocking_ostream;

}

#endif // !LOCKING_STREAM__H
