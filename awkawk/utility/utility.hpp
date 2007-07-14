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
//  Peter Bright <drpizza@anti-flash.co.uk>

#pragma once
#ifndef UTILITY__H
#define UTILITY__H

namespace utility
{

template<typename T, int N>
inline size_t array_size(T(&)[N])
{
	return N;
}

#define ARRAY_SIZE(X)	(sizeof(utility::array_size(X)) ? (sizeof(X) / sizeof((X)[0])) : 0)

template<typename InIter, typename OutIter, typename Pred>
OutIter copy_if(InIter begin, InIter end, OutIter dest, Pred p)
{
	while(begin != end)
	{
		if(p(*begin)) { *dest++ = *begin; }
		++begin;
	}
	return dest;
}

} // namespace utility

#endif // UTILITY__H
