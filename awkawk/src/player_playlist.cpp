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

#include "stdafx.h"
#include "player_playlist.h"
#include "util.h"

void player_playlist::dump_playlist() const
{
	for(playlist_type::const_iterator it(playlist.begin()), end(playlist.end()); it != end; ++it)
	{
		wdout << *it << L" ";
		if(it == playlist_start)
		{
			wdout << L"playlist_start ";
		}
		if(it == playlist_end)
		{
			wdout << L"playlist_end ";
		}
		if(it == playlist_position)
		{
			wdout << L"playlist_position ";
		}
		wdout << L"\n";
	}
	wdout << std::flush;
}


void player_playlist::do_next()
{
	if(after_end())
	{
		return;
	}
	
	switch(playlist_mode)
	{
	case repeat_single: // when the user presses 'next' in repeat single, we move forward normally
	case repeat_all:    // continue on wraparound
		{
			++playlist_position;
			if(after_end())
			{
				playlist_position = playlist_start;
				++playlist_position;
			}
		}
		break;
	case normal: // stop on wraparound
		{
			++playlist_position;
		}
		break;
	case shuffle:
		{
			// TODO implement shuffling
			++playlist_position;
			if(after_end())
			{
				playlist_position = playlist_start;
				++playlist_position;
			}
		}
		break;
	}
}

// transition from one track to the next
void player_playlist::do_transition()
{
	if(after_end())
	{
		return;
	}
	
	switch(playlist_mode)
	{
	case repeat_single:
		break;
	case repeat_all:    // continue on wraparound
		{
			++playlist_position;
			if(after_end())
			{
				playlist_position = playlist_start;
				++playlist_position;
			}
		}
		break;
	case normal: // stop on wraparound
		{
			++playlist_position;
		}
		break;
	case shuffle:
		{
			// TODO implement shuffling
			++playlist_position;
			if(after_end())
			{
				playlist_position = playlist_start;
				++playlist_position;
			}
		}
		break;
	}
}

void player_playlist::do_previous()
{
	if(before_start())
	{
		return;
	}

	switch(playlist_mode)
	{
	case repeat_single: // when the user presses 'next' in repeat single, we move forward normally
	case repeat_all:    // continue on wraparound
		{
			--playlist_position;
			if(before_start())
			{
				playlist_position = playlist_end;
				--playlist_position;
			}
		}
		break;
	case normal: // stop on wraparound
		{
			--playlist_position;
		}
		break;
	case shuffle:
		{
			// TODO implement shuffling
			--playlist_position;
			if(before_start())
			{
				playlist_position = playlist_end;
				--playlist_position;
			}
		}
		break;
	}
}
