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
#include "locks.hpp"

namespace utility
{
template<typename E, typename T = std::char_traits<E> >
struct locking_ostream
{
	utility::critical_section cs;
	std::basic_ostream<E, T>* s;

	locking_ostream(std::basic_ostream<E, T>& os) : s(&os)
	{
	}

	struct locking_ostream_helper
	{
		locking_ostream<E, T>* ls;

		locking_ostream_helper(locking_ostream<E, T>* l) : ls(l)
		{
			ls->cs.enter();
		}
		locking_ostream_helper(const locking_ostream_helper& rhs) : ls(rhs.ls)
		{
			ls->cs.enter();
		}
		~locking_ostream_helper()
		{
			ls->cs.leave();
		}

		template<typename T>
		locking_ostream_helper& operator<<(const T& rhs)
		{
			*(ls->s) << rhs;
			return *this;
		}
		locking_ostream_helper& operator<<(std::ostream& (*manip)(std::ostream&))
		{
			*(ls->s) << manip;
			return *this;
		}
	};

	template<typename T>
	locking_ostream_helper operator<<(const T& rhs)
	{
		locking_ostream_helper hlp(this);
		*s << rhs;
		return hlp;
	}
	locking_ostream_helper operator<<(std::ostream& (*manip)(std::ostream&))
	{
		locking_ostream_helper hlp(this);
		*s << manip;
		return hlp;
	}
};

}

#endif // !LOCKING_STREAM__H
