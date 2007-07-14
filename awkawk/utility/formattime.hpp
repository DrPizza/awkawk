// utility/formatTime.hpp

//  Copyright (C) 2002-2005 Peter Bright
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

#ifndef FORMATTIME__H
#define FORMATTIME__H
#include <string>
#include <ctime>
#include <sstream>
#include <locale>
#include <iostream>

#include <boost/scoped_array.hpp>

#include "utility/string.hpp"
#include "utility/iomanip.hpp"

namespace utility
{
	template<typename Elem>
	struct time_format_holder
	{
		static const int index;
		static void io_event(std::ios_base::event evt, std::ios_base& io, int)
		{
			switch(evt)
			{
			case std::ios_base::erase_event:
				{
					delete [] static_cast<Elem*>(io.pword(time_format_holder<Elem>::index));
					io.pword(time_format_holder<Elem>::index) = 0;
				}
				break;
			case std::ios_base::copyfmt_event:
				{
					// since the /copy/ now owns the array, I need to clone it for myself.
					Elem* fmt(static_cast<Elem*>(io.pword(time_format_holder<Elem>::index)));
					if(fmt != 0)
					{
						Elem* fmt_copy(new Elem[utility::string_length(fmt) + 1]);
						std::memcpy(fmt_copy, fmt, (utility::string_length(fmt) + 1) * sizeof(Elem));
						io.pword(time_format_holder<Elem>::index) = fmt_copy;
					}
				}
				break;
			case std::ios_base::imbue_event:
				break;
			}
		}
	};

	template<typename Elem>
	const int time_format_holder<Elem>::index = std::ios_base::xalloc();

	template<typename Elem>
	void set_time_format(std::ios_base& io, const Elem* fmt)
	{
		Elem* fmt_copy(new Elem[utility::string_length(fmt) + 1]);
		std::memcpy(fmt_copy, fmt, (utility::string_length(fmt) + 1) * sizeof(Elem));
		delete [] static_cast<Elem*>(io.pword(time_format_holder<Elem>::index));
		io.pword(time_format_holder<Elem>::index) = fmt_copy;
		if(io.iword(time_format_holder<Elem>::index) == 0)
		{
			io.register_callback(&time_format_holder<Elem>::io_event, time_format_holder<Elem>::index);
			io.iword(time_format_holder<Elem>::index) = 1;
		}
	}

	template<typename Elem>
	utility::smanip<const Elem*> settimeformat(const Elem* fmt)
	{
		return utility::smanip<const Elem*>(&set_time_format<Elem>, fmt);
	}

	template<typename Elem, typename Traits>
	std::basic_string<Elem, Traits> format_time(const std::basic_string<Elem, Traits>& format, const tm& time, const std::locale& loc = std::locale())
	{
		std::basic_ostringstream<Elem, Traits> ss;
		ss.imbue(loc);
		ss << settimeformat(format) << time;
		if(ss)
		{
			return ss.str();
		}
		else
		{
			return std::basic_string<Elem, Traits>();
		}
	}

	template<typename Elem>
	std::basic_string<Elem> format_time(const Elem* format, const tm& time, const std::locale& loc = std::locale())
	{
		std::basic_ostringstream<Elem> ss;
		ss.imbue(loc);
		ss << settimeformat(format) << time;
		if(ss)
		{
			return ss.str();
		}
		else
		{
			return std::basic_string<Elem>();
		}
	}
}

namespace std
{
	template<typename Elem, typename Traits>
	std::basic_ostream<Elem, Traits>& operator<<(std::basic_ostream<Elem, Traits>& os, const tm& datetime)
	{
		using namespace std;
		if(os)
		{
			static const Elem default_format[] = { static_cast<Elem>('%'), static_cast<Elem>('c'), static_cast<Elem>('\0') };
			//const Elem* format(static_cast<const Elem*>(os.pword(utility::time_format_holder<Elem>::index)));
			void* ptr(NULL);
			std::swap(os.pword(utility::time_format_holder<Elem>::index), ptr);
			const Elem* format(static_cast<const Elem*>(ptr));
			boost::scoped_array<const Elem> buffer(format);
			if(format == NULL)
			{
				format = default_format;
			}
			const Elem* format_end(format + utility::string_length(format));
			typedef ostreambuf_iterator<Elem, Traits> ostr_iter_type;
			const time_put<Elem, ostr_iter_type>& fmt(use_facet<time_put<Elem, ostr_iter_type> >(os.getloc()));
			if(fmt.put(ostr_iter_type(os), os, os.fill(), &datetime, format, format_end).failed())
			{
				os.setstate(ios_base::badbit);
			}
		}
		return os;
	}
}

#endif // #ifndef FORMATTIME__H
