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

#pragma once

#ifndef PLAYER_MENUS__H
#define PLAYER_MENUS__H

#include "stdafx.h"

#include "menu.h"
#include "resource.h"

struct awkawk;

struct awkawk_menu : menu
{
	awkawk_menu(awkawk* player_, window* owner, menu* parent, const std::wstring& caption);
protected:
	awkawk_menu(awkawk* player_, window* owner, const std::wstring& caption);
	awkawk* player;
};

struct awkawk_play_menu : awkawk_menu
{
	awkawk_play_menu(awkawk* player, window* owner, awkawk_menu* parent) : awkawk_menu(player, owner, parent, L"&Play"), menu_item(parent)
	{
		append(new menu_entry(this, IDM_PAUSE, L"&Play/Pause\tSpace"));
		append(new menu_entry(this, IDM_PLAY, L"P&lay"));
		append(new menu_entry(this, IDM_STOP, L"&Stop"));
	}

	virtual void onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu);
};

struct awkawk_mode_menu : awkawk_menu
{
	awkawk_mode_menu(awkawk* player, window* owner, awkawk_menu* parent) : awkawk_menu(player, owner, parent, L"Play &Mode"), menu_item(parent)
	{
		append(new menu_entry(this, IDM_PLAYMODE_NORMAL, L"&Normal"));
		append(new menu_entry(this, IDM_PLAYMODE_REPEATALL, L"Repeat &All"));
		append(new menu_entry(this, IDM_PLAYMODE_REPEATTRACK, L"Repeat &Track"));
		append(new menu_entry(this, IDM_PLAYMODE_SHUFFLE, L"&Shuffle"));
	}

	virtual void onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu);
};

struct awkawk_size_menu : awkawk_menu
{
	enum
	{
		ar_menu_base = WM_USER + 0x200,
		lb_menu_base = WM_USER + 0x300
	};

	awkawk_size_menu(awkawk* player, window* owner, awkawk_menu* parent) : awkawk_menu(player, owner, parent, L"&Size"), menu_item(parent)
	{
		append(new menu_entry(this, IDM_SIZE_50, L"&50%\tAlt+1"));
		append(new menu_entry(this, IDM_SIZE_100, L"&100%\tAlt+2"));
		append(new menu_entry(this, IDM_SIZE_200, L"&200%\tAlt+3"));
		append(new menu_entry(this, IDM_SIZE_FREE, L"&Free\tAlt+4"));
		append(new menu_separator(this));
		// AR entries
		append(new menu_separator(this));
		// letterbox entries
	}

	virtual LRESULT CALLBACK message_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
	virtual void onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu);
	virtual void onUnInitMenuPopup(HWND wnd, HMENU menu, WORD type);
	virtual void onCommand(HWND wnd, int id, HWND control, UINT event);
};

struct awkawk_filter_menu : awkawk_menu
{
	awkawk_filter_menu(awkawk* player, window* owner, awkawk_menu* parent) : awkawk_menu(player, owner, parent, L"&Filters"), menu_item(parent)
	{
	}

	enum
	{
		filter_menu_base = WM_USER + 0x100
	};

	virtual LRESULT CALLBACK message_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
	virtual void onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu);
	virtual void onUnInitMenuPopup(HWND wnd, HMENU menu, WORD type);
	virtual void onCommand(HWND wnd, int id, HWND control, UINT event);
};

struct awkawk_main_menu : awkawk_menu
{
	awkawk_main_menu(awkawk* player, window* owner) : awkawk_menu(player, owner, L"awkawk")
	{
		append(new menu_entry(this, IDM_OPEN_FILE, L"&Open File...\tCtrl+O"));
		append(new menu_entry(this, IDM_OPEN_URL, L"Open &URL...\tCtrl+U"));
		append(new menu_separator(this));
		append(new awkawk_play_menu(player, owner, this));
		append(new awkawk_mode_menu(player, owner, this));
		append(new awkawk_size_menu(player, owner, this));
		append(new awkawk_filter_menu(player, owner, this));
		append(new menu_separator(this));
		append(new menu_entry(this, IDM_CLOSE_FILE, L"&Close File\tCtrl+W"));
		append(new menu_entry(this, IDM_EXIT, L"E&xit"));
	}

	virtual void onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu);
};

#endif
