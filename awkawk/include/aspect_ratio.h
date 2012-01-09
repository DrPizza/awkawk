//  Copyright (C) 2012 Peter Bright
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

#ifndef ASPECT_RATIO__H
#define ASPECT_RATIO__H

#include "stdafx.h"

#include <string>
#include <boost/rational.hpp>

#include "util.h"

SIZE fix_ar_width(SIZE current_size, rational_type factor);

SIZE fix_ar_height(SIZE current_size, rational_type factor);

SIZE fix_ar_enlarge(SIZE current_size, rational_type actual, rational_type intended);

SIZE fix_ar_shrink(SIZE current_size, rational_type actual, rational_type intended);

SIZE fix_ar(SIZE current_size, rational_type actual, rational_type intended, bool size_is_maximum);

SIZE scale_size(SIZE current_size, rational_type factor);

struct awkawk;

struct aspect_ratio
{
	virtual rational_type get_multiplier() const = 0;

	virtual const std::wstring get_name() const = 0;

	virtual ~aspect_ratio()
	{
	}
};

struct fixed_aspect_ratio : aspect_ratio
{
	fixed_aspect_ratio(rational_type::int_type width_, rational_type::int_type height_) : ratio(width_, height_)
	{
	}

	fixed_aspect_ratio(rational_type::int_type width_, rational_type::int_type height_, std::wstring name_) : ratio(width_, height_), name(name_)
	{
	}

	virtual rational_type get_multiplier() const
	{
		return ratio;
	}

	virtual const std::wstring get_name() const
	{
		if(name.empty())
		{
			std::wstringstream wss;
			wss << ratio.numerator() << L":" << ratio.denominator();
			return wss.str();
		}
		else
		{
			return name;
		}
	}

private:
	rational_type ratio;
	std::wstring name;
};

struct natural_aspect_ratio : aspect_ratio
{
	natural_aspect_ratio(awkawk* player_) : player(player_)
	{
	}

	virtual rational_type get_multiplier() const;

	virtual const std::wstring get_name() const;

private:
	awkawk* player;
};

struct letterbox
{
	virtual rational_type get_multiplier() const = 0;

	virtual const std::wstring get_name() const = 0;

	virtual ~letterbox()
	{
	}
};

struct fixed_letterbox : letterbox
{
	fixed_letterbox(rational_type::int_type width_, rational_type::int_type height_) : ratio(width_, height_)
	{
	}

	fixed_letterbox(rational_type::int_type width_, rational_type::int_type height_, std::wstring name_) : ratio(width_, height_), name(name_)
	{
	}

	virtual rational_type get_multiplier() const
	{
		return ratio;
	}

	virtual const std::wstring get_name() const
	{
		if(name.empty())
		{
			std::wstringstream wss;
			wss << ratio.numerator() << L":" << ratio.denominator() << L" Original";
			return wss.str();
		}
		else
		{
			return name + L" Original";
		}
	}

private:
	rational_type ratio;
	std::wstring name;
};

struct natural_letterbox : letterbox
{
	natural_letterbox(awkawk* player_) : player(player_)
	{
	}

	virtual rational_type get_multiplier() const;

	virtual const std::wstring get_name() const;

private:
	awkawk* player;
};

#endif
