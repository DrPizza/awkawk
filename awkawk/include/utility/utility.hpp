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

inline bool is_nan(float f)
{
	return (*reinterpret_cast<unsigned __int32*>(&f) & 0x7f800000) == 0x7f800000 && (*reinterpret_cast<unsigned __int32*>(&f) & 0x007fffff) != 0;
}

inline bool is_finite(float f)
{
	return (*reinterpret_cast<unsigned __int32*>(&f) & 0x7f800000) != 0x7f800000;
}

// if this symbol is defined, NaNs are never equal to anything (as is normal in IEEE floating point)
// if this symbol is not defined, NaNs are hugely different from regular numbers, but might be equal to each other
#define UNEQUAL_NANS 1
// if this symbol is defined, infinites are never equal to finite numbers (as they're unimaginably greater)
// if this symbol is not defined, infinities are 1 ULP away from +/- FLT_MAX
#define INFINITE_INFINITIES 1

// test whether two IEEE floats are within a specified number of representable values of each other
// This depends on the fact that IEEE floats are properly ordered when treated as signed magnitude integers
inline bool equal_float(float lhs, float rhs, unsigned __int32 max_ulp_difference)
{
#ifdef UNEQUAL_NANS
	if(is_nan(lhs) || is_nan(rhs))
	{
		return false;
	}
#endif
#ifdef INFINITE_INFINITIES
	if((is_finite(lhs) && !is_finite(rhs)) || (!is_finite(lhs) && is_finite(rhs)))
	{
		return false;
	}
#endif
	signed __int32 left(*reinterpret_cast<signed __int32*>(&lhs));
	// transform signed magnitude ints into 2s complement signed ints
	if(left < 0)
	{
		left = 0x80000000 - left;
	}
	signed __int32 right(*reinterpret_cast<signed __int32*>(&rhs));
	// transform signed magnitude ints into 2s complement signed ints
	if(right < 0)
	{
		right = 0x80000000 - right;
	}
	if(static_cast<unsigned __int32>(std::abs(left - right)) <= max_ulp_difference)
	{
		return true;
	}
	return false;
}

} // namespace utility

#endif // UTILITY__H
