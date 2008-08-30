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

#ifndef IOMANIP__HPP
#define IOMANIP__HPP

#include <iosfwd>

namespace utility
{

template<typename Arg>
struct smanip
{
	smanip(void(* fun_)(std::ios_base&, Arg), Arg arg_) : fun(fun_), arg(arg_)
	{}
	void (* fun)(std::ios_base&, Arg);
	Arg arg;
};

template<typename Elem, typename Traits, typename Arg>
std::basic_istream<Elem, Traits>& operator>>(std::basic_istream<Elem, Traits>& is, const utility::smanip<Arg>& man)
{
	(*man.fun)(is, man.arg);
	return is;
}

template<typename Elem, typename Traits, typename Arg>
std::basic_ostream<Elem, Traits>& operator<<(std::basic_ostream<Elem, Traits>& os, const utility::smanip<Arg>& man)
{
	(*man.fun)(os, man.arg);
	return os;
}

}

#endif // #ifndef IOMANIP_HPP