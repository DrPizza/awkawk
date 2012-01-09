#include "stdafx.h"

#include "aspect_ratio.h"
#include "awkawk.h"

SIZE fix_ar_width(SIZE current_size, rational_type factor)
{
	current_size.cx = static_cast<LONG>((static_cast<rational_type::int_type>(current_size.cy) * factor.numerator()) / factor.denominator());
	return current_size;
}

SIZE fix_ar_height(SIZE current_size, rational_type factor)
{
	current_size.cy = static_cast<LONG>((static_cast<rational_type::int_type>(current_size.cx) * factor.denominator()) / factor.numerator());
	return current_size;
}

SIZE fix_ar_enlarge(SIZE current_size, rational_type actual, rational_type intended)
{
	if(actual > intended) {
		current_size.cy = static_cast<LONG>((static_cast<rational_type::int_type>(current_size.cx) * intended.denominator()) / intended.numerator());
	} else if(actual < intended) {
		current_size.cx = static_cast<LONG>((static_cast<rational_type::int_type>(current_size.cy) * intended.numerator()) / intended.denominator());
	}
	return current_size;
}

SIZE fix_ar_shrink(SIZE current_size, rational_type actual, rational_type intended)
{
	if(actual > intended) {
		current_size.cx = static_cast<LONG>((static_cast<rational_type::int_type>(current_size.cy) * intended.numerator()) / intended.denominator());
	} else if(actual < intended) {
		current_size.cy = static_cast<LONG>((static_cast<rational_type::int_type>(current_size.cx) * intended.denominator()) / intended.numerator());
	}
	return current_size;
}

SIZE fix_ar(SIZE current_size, rational_type actual, rational_type intended, bool size_is_maximum)
{
	return size_is_maximum ? fix_ar_shrink(current_size, actual, intended) : fix_ar_enlarge(current_size, actual, intended);
}

SIZE scale_size(SIZE current_size, rational_type factor)
{
	current_size.cx = static_cast<LONG>((static_cast<rational_type::int_type>(current_size.cx) * factor.numerator()) / factor.denominator());
	current_size.cy = static_cast<LONG>((static_cast<rational_type::int_type>(current_size.cy) * factor.numerator()) / factor.denominator());
	return current_size;
}

rational_type natural_aspect_ratio::get_multiplier() const
{
	SIZE sz(player->get_video_dimensions());
	return rational_type(sz.cx, sz.cy);
}

const std::wstring natural_aspect_ratio::get_name() const
{
	std::wstringstream wss;
	SIZE sz(player->get_video_dimensions());
	rational_type r(get_multiplier());
	wss << L"Original " << r.numerator() << L":" << r.denominator() << L" (" << sz.cx << L" x " << sz.cy << L")";
	return wss.str();
}

rational_type natural_letterbox::get_multiplier() const
{
	return player->available_ratios[player->get_aspect_ratio_mode()]->get_multiplier();
}

const std::wstring natural_letterbox::get_name() const
{
	std::wstringstream wss;
	SIZE sz(player->get_video_dimensions());
	rational_type r(get_multiplier());
	wss << L"No letterboxing " << r.numerator() << L":" << r.denominator() << L" (" << sz.cx << L" x " << sz.cy << L")";
	return wss.str();
}
