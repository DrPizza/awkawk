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
#ifdef TRACK_LOCKS
lock_tracker tracker;
#endif

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

std::set<utility::lock_tracker::cs_sequence> get_all_sequences(const std::list<utility::lock_tracker::lock_manipulation>& l)
{
	std::set<utility::lock_tracker::cs_sequence> lock_seq;
	std::list<lock_tracker::lock_manipulation> current_sequence;
	for(std::list<lock_tracker::lock_manipulation>::const_iterator lit(l.begin()), lend(l.end()); lit != lend; ++lit)
	{
		switch(lit->operation)
		{
		case lock_tracker::lock_manipulation::attempt:
			current_sequence.push_back(*lit);
			break;
		case lock_tracker::lock_manipulation::release:
			{
				if(current_sequence.rend() == std::find_if(current_sequence.rbegin(), current_sequence.rend(), std::bind1st(section_equality(), *lit)))
				{
					std::cerr << "warning: the section at " << lit->section << " was unlocked more times than it was locked" << std::endl;
				}
				else
				{
					current_sequence.erase(--(std::find_if(current_sequence.rbegin(), current_sequence.rend(), std::bind1st(section_equality(), *lit)).base()));
				}
			}
			break;
		}
		lock_seq.insert(utility::lock_tracker::cs_sequence(current_sequence, &l));
	}
	return lock_seq;
}

std::set<std::pair<lock_tracker::cs_sequence, lock_tracker::cs_sequence> > lock_tracker::analyze_deadlocks()
{
	using std::set;
	using std::list;
	using std::map;
	using std::find_if;
	using std::find_first_of;
	using std::remove_if;
	using std::bind1st;
	using std::pair;

	::EnterCriticalSection(&cs);
	ON_BLOCK_EXIT(::LeaveCriticalSection(&cs));
	set<cs_sequence> lock_seq;
	for(list<list<lock_tracker::lock_manipulation> >::const_iterator it(lock_sequences.begin()), end(lock_sequences.end()); it != end; ++it)
	{
		set<cs_sequence> seq(get_all_sequences(*it));
		lock_seq.insert(seq.begin(), seq.end());
	}
	for(map<DWORD, std::shared_ptr<utility::lock_tracker::lock_info> >::const_iterator it(info.begin()), end(info.end()); it != end; ++it)
	{
		set<cs_sequence> seq(get_all_sequences(it->second->current_sequence));
		lock_seq.insert(seq.begin(), seq.end());
	}

	list<cs_sequence> cleaned_sequences;
	// remove recursively held locks, because for deadlock detection, they don't matter; critical sections are recursive
	for(set<cs_sequence>::iterator needle(lock_seq.begin()), nend(lock_seq.end()); needle != nend; ++needle)
	{
		cs_sequence& need(const_cast<cs_sequence&>(*needle));
		
		for(list<lock_tracker::lock_manipulation>::iterator it(need.sequence.begin()); it != need.sequence.end() && utility::advance(it, 1) != need.sequence.end(); ++it)
		{
			need.sequence.erase(remove_if(utility::advance(it, 1), need.sequence.end(), bind1st(section_equality(), *it)), need.sequence.end());
		}
		cleaned_sequences.push_back(need);
	}

	set<pair<cs_sequence, cs_sequence> > deadlocks;
	typedef list<cs_sequence>::const_iterator sequence_list_c_iterator;
	for(sequence_list_c_iterator needle(cleaned_sequences.begin()), nend(cleaned_sequences.end()); needle != nend; ++needle)
	{
		for(sequence_list_c_iterator haystack(needle), hend(cleaned_sequences.end()); haystack != nend; ++haystack)
		{
			typedef list<lock_tracker::lock_manipulation>::const_iterator manipulation_list_c_iterator;
			for(manipulation_list_c_iterator lit(needle->sequence.begin()), lend(needle->sequence.end()); lit != lend; ++lit)
			{
				manipulation_list_c_iterator midpoint(find_if(haystack->sequence.begin(), haystack->sequence.end(), bind1st(section_equality(), *lit)));
				if(midpoint != haystack->sequence.end())
				{
					if(find_first_of(midpoint, haystack->sequence.end(), needle->sequence.begin(), lit, section_equality()) != haystack->sequence.end())
					{
						deadlocks.insert(pair<cs_sequence, cs_sequence>(*needle, *haystack));
					}
				}
			}
		}
	}
	return deadlocks;
}

}
