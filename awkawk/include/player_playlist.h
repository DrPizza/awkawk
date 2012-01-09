//  Copyright (C) 2008 Peter Bright
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

#ifndef PLAYER_PLAYLIST__H
#define PLAYER_PLAYLIST__H

#include "stdafx.h"
#include "resource.h"

struct player_playlist
{
	player_playlist() : playlist(),
	                    playlist_mode(normal)
	{
		playlist.push_back(L"<START>");
		playlist_start = playlist.begin();
		playlist.push_back(L"<END>");
		playlist_end = ++playlist.begin();
		playlist_position = playlist_start;
	}

	typedef std::list<std::wstring> playlist_type;

	enum play_mode
	{
		normal = IDM_PLAYMODE_NORMAL, // advance through playlist, stop & rewind when the last file is played
		repeat_all = IDM_PLAYMODE_REPEATALL, // advance through the playlist, rewind & continue playing when the last file is played
		repeat_single = IDM_PLAYMODE_REPEATTRACK, // do not advance through playlist, rewind track & continue playing
		shuffle = IDM_PLAYMODE_SHUFFLE // advance through playlist randomly, no concept of a "last file"
	};

	play_mode get_playmode() const
	{
		return playlist_mode;
	}

	void set_playmode(play_mode pm)
	{
		playlist_mode = pm;
	}

	void dump_playlist() const;

	void add_file(const std::wstring& path)
	{
		playlist.insert(playlist_end, path);
		dump_playlist();
	}

	void clear_files()
	{
		playlist_type::iterator start(playlist_start);
		++start;
		playlist.erase(start, playlist_end);
		playlist_position = playlist_start;
		dump_playlist();
	}

	std::wstring get_file_name() const
	{
		return *playlist_position;
	}

	size_t count() const
	{
		return playlist.size() - 2;
	}

	bool empty() const
	{
		return count() == 0;
	}

	bool before_start() const
	{
		return playlist_position == playlist_start;
	}

	bool after_end() const
	{
		return playlist_position == playlist_end;
	}

	void do_next();
	// transition from one track to the next
	void do_transition();
	void do_previous();

private:
	playlist_type playlist;
	playlist_type::iterator playlist_position;
	playlist_type::iterator playlist_start;
	playlist_type::iterator playlist_end;
	play_mode playlist_mode;
};

#endif
