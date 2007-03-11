//  Copyright (C) 2006 Peter Bright
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

#include "locks.hpp"

#include <functional>
#include <iostream>

namespace utility
{
lock_tracker tracker;

namespace
{
	struct section_equality : std::binary_function<lock_tracker::lock_manipulation, lock_tracker::lock_manipulation, bool>
	{
		bool operator()(const lock_tracker::lock_manipulation& lhs, const lock_tracker::lock_manipulation& rhs) const
		{
			return lhs.section == rhs.section;
		}
	};
}

std::set<std::pair<lock_tracker::cs_sequence, lock_tracker::cs_sequence> > lock_tracker::analyze_deadlocks()
{
	::EnterCriticalSection(&cs);
	ON_BLOCK_EXIT(&::LeaveCriticalSection, &cs);
	std::set<cs_sequence> lock_seq;
	for(std::list<std::list<lock_tracker::lock_manipulation> >::iterator it(lock_sequences.begin()), end(lock_sequences.end()); it != end; ++it)
	{
		std::list<lock_tracker::lock_manipulation> current_sequence;
		for(std::list<lock_tracker::lock_manipulation>::const_iterator lit(it->begin()), lend(it->end()); lit != lend; ++lit)
		{
			switch(lit->operation)
			{
			case lock_tracker::lock_manipulation::acquire:
				current_sequence.push_back(*lit);
				break;
			case lock_tracker::lock_manipulation::release:
				{
					current_sequence.erase(--(std::find_if(current_sequence.rbegin(), current_sequence.rend(), std::bind1st(section_equality(), *lit)).base()));
				}
				break;
			}
			lock_seq.insert(cs_sequence(current_sequence, &*it));
		}
	}

	std::vector<cs_sequence> cleaned_sequences;
	// remove recursively held locks, because for deadlock detection, they don't matter.
	for(std::set<cs_sequence>::iterator needle(lock_seq.begin()), nend(lock_seq.end()); needle != nend; ++needle)
	{
		for(std::list<lock_tracker::lock_manipulation>::iterator it(needle->sequence.begin()); it != needle->sequence.end() && utility::advance(it, 1) != needle->sequence.end();)
		{
			std::list<lock_tracker::lock_manipulation>::iterator next(utility::advance(it, 1));
			std::list<lock_tracker::lock_manipulation>::iterator pos(std::find_if(next, needle->sequence.end(), std::bind1st(section_equality(), *it)));
			if(pos != needle->sequence.end())
			{
				needle->sequence.erase(pos);
			}
			else
			{
				++it;
			}
		}
		cleaned_sequences.push_back(*needle);
	}

	std::set<std::pair<cs_sequence, cs_sequence> > deadlocks;
	for(std::vector<cs_sequence>::const_iterator needle(cleaned_sequences.begin()), nend(cleaned_sequences.end()); needle != nend; ++needle)
	{
		for(std::vector<cs_sequence>::const_iterator haystack(needle), hend(cleaned_sequences.end()); haystack != nend; ++haystack)
		{
			for(std::list<lock_tracker::lock_manipulation>::const_iterator lit(needle->sequence.begin()), lend(needle->sequence.end()); lit != lend; ++lit)
			{
				std::list<lock_tracker::lock_manipulation>::const_iterator midpoint(std::find_if(haystack->sequence.begin(), haystack->sequence.end(), std::bind1st(section_equality(), *lit)));
				if(midpoint != haystack->sequence.end())
				{
					if(std::find_first_of(midpoint, haystack->sequence.end(), needle->sequence.begin(), lit, section_equality()) != haystack->sequence.end())
					{
						deadlocks.insert(std::pair<cs_sequence, cs_sequence>(*needle, *haystack));
					}
				}
			}
		}
	}
	return deadlocks;
}

}
