//  Copyright (C) 2007 Peter Bright
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

#ifndef PLAYER_MENUS__H
#define PLAYER_MENUS__H

#include "stdafx.h"

#include "menu.h"
#include "resource.h"

struct awkawk;

struct awkawk_menu : menu
{
	awkawk_menu(awkawk* player_, window* owner, const wchar_t* caption);
	awkawk_menu(awkawk* player_, window* owner, const wchar_t* caption, menu* parent);
	awkawk* player;
};

struct awkawk_play_menu : awkawk_menu
{
	awkawk_play_menu(awkawk* player, window* owner, awkawk_menu* parent) : awkawk_menu(player, owner, L"&Play", parent)
	{
		append(new menu_string(IDM_PAUSE, L"&Play/Pause\tSpace"));
		append(new menu_string(IDM_PLAY, L"P&lay"));
		append(new menu_string(IDM_STOP, L"&Stop"));
		construct_menu();
	}

	virtual void onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu);
	virtual void onCommand(HWND wnd, int id, HWND control, UINT event);
};

struct awkawk_mode_menu : awkawk_menu
{
	awkawk_mode_menu(awkawk* player, window* owner, awkawk_menu* parent) : awkawk_menu(player, owner, L"Play &Mode", parent)
	{
		append(new menu_string(IDM_PLAYMODE_NORMAL, L"&Normal"));
		append(new menu_string(IDM_PLAYMODE_REPEATALL, L"Repeat &All"));
		append(new menu_string(IDM_PLAYMODE_REPEATTRACK, L"Repeat &Track"));
		append(new menu_string(IDM_PLAYMODE_SHUFFLE, L"&Shuffle"));
		construct_menu();
	}

	virtual void onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu);
	virtual void onCommand(HWND wnd, int id, HWND control, UINT event);
};

struct awkawk_size_menu : awkawk_menu
{
	awkawk_size_menu(awkawk* player, window* owner, awkawk_menu* parent) : awkawk_menu(player, owner, L"&Size", parent)
	{
		//append(new menu_string(IDM_SIZE_50, L"&50%\tAlt+1"));
		//append(new menu_string(IDM_SIZE_100, L"&100%\tAlt+2"));
		//append(new menu_string(IDM_SIZE_200, L"&200%\tAlt+3"));
		//append(new menu_string(IDM_SIZE_FREE, L"&Free\tAlt+4"));
		//append(new menu_separator());
		//append(new menu_string(IDM_AR_ORIGINAL, L"&Original AR\tShift+1"));
		//append(new menu_string(IDM_AR_133TO1, L"4:3\tShift+2"));
		//append(new menu_string(IDM_AR_155TO1, L"14:9\tShift+3"));
		//append(new menu_string(IDM_AR_177TO1, L"16:9\tShift+4"));
		//append(new menu_string(IDM_AR_185TO1, L"1.85:1\tShift+5"));
		//append(new menu_string(L"2.40:1\tShift+6", IDM_AR_240TO1));
		//append(new menu_string(L"Custom", IDM_AR_CUSTOM));
		//append(new menu_separator());
		//append(new menu_string(IDM_NOLETTERBOXING, L"&No Letterboxing\tCtrl+1"));
		//append(new menu_string(IDM_4_TO_3_ORIGINAL, L"4:3 Original\tCtrl+2"));
		//append(new menu_string(IDM_14_TO_9_ORIGINAL, L"14:9 Original\tCtrl+3"));
		//append(new menu_string(IDM_16_TO_9_ORIGINAL, L"16:9 Original\tCtrl+4"));
		//append(new menu_string(IDM_185_TO_1_ORIGINAL, L"1.85:1 Original\tCtrl+5"));
		//append(new menu_string(IDM_240_TO_1_ORIGINAL, L"2.40:1 Original\tCtrl+6"));

		append(new themed_string_item(owner, IDM_SIZE_50, this, L"&50%\tAlt+1"));
		append(new themed_string_item(owner, IDM_SIZE_100, this, L"&100%\tAlt+2"));
		append(new themed_string_item(owner, IDM_SIZE_200, this, L"&200%\tAlt+3"));
		append(new themed_string_item(owner, IDM_SIZE_FREE, this, L"&Free\tAlt+4"));
		append(new themed_separator_item(owner, IDM_SIZE_SEPARATOR_1, this));
		append(new themed_string_item(owner, IDM_AR_ORIGINAL, this, L"&Original AR\tShift+1"));
		append(new themed_string_item(owner, IDM_AR_133TO1, this, L"4:3\tShift+2"));
		append(new themed_string_item(owner, IDM_AR_155TO1, this, L"14:9\tShift+3"));
		append(new themed_string_item(owner, IDM_AR_177TO1, this, L"16:9\tShift+4"));
		append(new themed_string_item(owner, IDM_AR_185TO1, this, L"1.85:1\tShift+5"));
		append(new themed_string_item(owner, IDM_AR_240TO1, this, L"2.40:1\tShift+6"));
		append(new themed_string_item(owner, IDM_AR_CUSTOM, this, L"Custom"));
		append(new themed_separator_item(owner, IDM_SIZE_SEPARATOR_2, this));
		append(new themed_string_item(owner, IDM_NOLETTERBOXING, this, L"&No Letterboxing\tCtrl+1"));
		append(new themed_string_item(owner, IDM_4_TO_3_ORIGINAL, this, L"4:3 Original\tCtrl+2"));
		append(new themed_string_item(owner, IDM_14_TO_9_ORIGINAL, this, L"14:9 Original\tCtrl+3"));
		append(new themed_string_item(owner, IDM_16_TO_9_ORIGINAL, this, L"16:9 Original\tCtrl+4"));
		append(new themed_string_item(owner, IDM_185_TO_1_ORIGINAL, this, L"1.85:1 Original\tCtrl+5"));
		append(new themed_string_item(owner, IDM_240_TO_1_ORIGINAL, this, L"2.40:1 Original\tCtrl+6"));
		construct_menu();
	}

	virtual void onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu);
	virtual void onCommand(HWND wnd, int id, HWND control, UINT event);
};

struct awkawk_filter_menu : awkawk_menu
{
	awkawk_filter_menu(awkawk* player, window* owner, awkawk_menu* parent) : awkawk_menu(player, owner, L"&Filters", parent)
	{
		construct_menu();
	}
	enum
	{
		filter_menu_base = WM_USER + 0x100
	};

	virtual bool handles_message(HWND window, UINT message, WPARAM wParam, LPARAM lParam) const
	{
		switch(message)
		{
		case WM_COMMAND:
			{
				if(HIWORD(wParam) == 0
				&& LOWORD(wParam) < (1 << 15)
				&& LOWORD(wParam) >= filter_menu_base)
				{
					return true;
				}
			}
			break;
		}
		return awkawk_menu::handles_message(window, message, wParam, lParam);
	}

	virtual LRESULT CALLBACK message_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)
		{
		case WM_COMMAND:
			{
				if(HIWORD(wParam) == 0
				&& LOWORD(wParam) < (1 << 15)
				&& LOWORD(wParam) >= filter_menu_base)
				{
					return HANDLE_WM_COMMAND(window, wParam, lParam, onCommand);
				}
			}
			break;
		}
		return awkawk_menu::message_proc(window, message, wParam, lParam);
	}

	void build_filter_menu(UINT position);
	bool show_filter_properties(size_t chosen) const;

	virtual void onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu);
	virtual void onCommand(HWND wnd, int id, HWND control, UINT event);
};

struct awkawk_main_menu : awkawk_menu
{
	awkawk_main_menu(awkawk* player, window* owner) : awkawk_menu(player, owner, L"awkawk")
	{
		append(new menu_string(IDM_OPEN_FILE, L"&Open File...\tCtrl+O"));
		append(new menu_string(IDM_OPEN_URL, L"Open &URL...\tCtrl+U"));
		append(new menu_separator());
		append(new awkawk_play_menu(player, owner, this));
		append(new awkawk_mode_menu(player, owner, this));
		append(new awkawk_size_menu(player, owner, this));
		append(new awkawk_filter_menu(player, owner, this));
		append(new menu_separator());
		append(new menu_string(IDM_CLOSE_FILE, L"&Close File\tCtrl+W"));
		append(new menu_string(IDM_EXIT, L"E&xit"));
		construct_menu();
	}

	virtual void onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu);
	virtual void onCommand(HWND wnd, int id, HWND control, UINT event);
};

#endif
