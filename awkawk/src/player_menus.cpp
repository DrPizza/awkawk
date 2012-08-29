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

#include "stdafx.h"

#include "player_menus.h"
#include "awkawk.h"
#include "player_direct_show.h"

awkawk_menu::awkawk_menu(awkawk* player_, window* owner, const std::wstring& caption) : menu(owner, 0, caption), player(player_)
{
}

awkawk_menu::awkawk_menu(awkawk* player_, window* owner, menu* parent, const std::wstring& caption) : menu(owner, parent, 0, caption), player(player_), menu_item(parent)
{
}

void awkawk_play_menu::onInitMenuPopup(HWND, HMENU, UINT, BOOL)
{
	::EnableMenuItem(get_menu(), IDM_PLAY, player->permitted(awkawk::play) ? MF_ENABLED : MF_GRAYED);
	::EnableMenuItem(get_menu(), IDM_PAUSE, player->permitted(awkawk::pause) ? MF_ENABLED : MF_GRAYED);
	::EnableMenuItem(get_menu(), IDM_STOP, player->permitted(awkawk::stop) ? MF_ENABLED : MF_GRAYED);
}

void awkawk_mode_menu::onInitMenuPopup(HWND, HMENU, UINT, BOOL)
{
	::EnableMenuItem(get_menu(), IDM_PLAYMODE_NORMAL, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_PLAYMODE_REPEATALL, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_PLAYMODE_REPEATTRACK, MF_ENABLED);
	//::EnableMenuItem(playmode_menu, IDM_PLAYMODE_SHUFFLE, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_PLAYMODE_SHUFFLE, MF_GRAYED);
	::CheckMenuRadioItem(get_menu(), IDM_PLAYMODE_NORMAL, IDM_PLAYMODE_SHUFFLE, player->get_playmode(), MF_BYCOMMAND);
}

LRESULT CALLBACK awkawk_size_menu::message_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = false;
	switch(message)
	{
	case WM_COMMAND:
		{
			if(HIWORD(wParam) == 0
			&& ((LOWORD(wParam) >= ar_menu_base && LOWORD(wParam) < ar_menu_base + player->available_ratios.size())
			||  (LOWORD(wParam) >= lb_menu_base && LOWORD(wParam) < lb_menu_base + player->available_letterboxes.size())))
			{
				handled = true;
				return HANDLE_WM_COMMAND(window, wParam, lParam, onCommand);
			}
		}
		break;
	}
	return awkawk_menu::message_proc(window, message, wParam, lParam, handled);
}

void awkawk_size_menu::onInitMenuPopup(HWND, HMENU, UINT, BOOL)
{
	::EnableMenuItem(get_menu(), IDM_SIZE_50, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_SIZE_100, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_SIZE_200, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_SIZE_FREE, MF_ENABLED);
	::CheckMenuRadioItem(get_menu(), IDM_SIZE_50, IDM_SIZE_FREE, player->get_window_size_mode(), MF_BYCOMMAND);

	UINT position(5);
	UINT ar_id(ar_menu_base);
	for(size_t i(0); i < player->available_ratios.size(); ++i)
	{
		insert(new menu_entry(this, ar_id++, player->available_ratios[i]->get_name()), position++);
	}
	::CheckMenuRadioItem(get_menu(), 5, 5 + player->available_ratios.size(), 5 + player->get_aspect_ratio_mode(), MF_BYPOSITION);

	++position;
	UINT letterbox_base(position);
	UINT lb_id(lb_menu_base);
	for(size_t i(0); i < player->available_letterboxes.size(); ++i)
	{
		insert(new menu_entry(this, lb_id++, player->available_letterboxes[i]->get_name()), position++);
	}
	::CheckMenuRadioItem(get_menu(), letterbox_base, letterbox_base + player->available_letterboxes.size(), letterbox_base + player->get_letterbox_mode(), MF_BYPOSITION);
}

void awkawk_size_menu::onUnInitMenuPopup(HWND, HMENU, WORD)
{
	for(size_t i(0); i < player->available_ratios.size(); ++i)
	{
		remove(5);
	}
	for(size_t i(0); i < player->available_letterboxes.size(); ++i)
	{
		remove(6);
	}
}

void awkawk_size_menu::onCommand(HWND, int id, HWND, UINT)
{
	if(static_cast<UINT>(id) >= ar_menu_base && static_cast<UINT>(id) < ar_menu_base + player->available_ratios.size())
	{
		player->set_aspect_ratio_mode(id - ar_menu_base);
	}
	if(static_cast<UINT>(id) >= lb_menu_base && static_cast<UINT>(id) < lb_menu_base + player->available_letterboxes.size())
	{
		player->set_letterbox_mode(id - lb_menu_base);
	}
}

LRESULT CALLBACK awkawk_filter_menu::message_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = false;
	switch(message)
	{
	case WM_COMMAND:
		{
			if(HIWORD(wParam) == 0
			&& LOWORD(wParam) >= filter_menu_base
			&& LOWORD(wParam) <  filter_menu_base + player->get_filters().size())
			{
				handled = true;
				return HANDLE_WM_COMMAND(window, wParam, lParam, onCommand);
			}
		}
		break;
	}
	return awkawk_menu::message_proc(window, message, wParam, lParam, handled);
}

void awkawk_filter_menu::onInitMenuPopup(HWND, HMENU, UINT, BOOL)
{
	if(player->permitted(awkawk::play) || player->permitted(awkawk::pause) || player->permitted(awkawk::stop))
	{
		std::vector<ATL::CAdapt<IBaseFilterPtr> > filters(player->get_filters());
		UINT flt_id(filter_menu_base);
		for(std::vector<ATL::CAdapt<IBaseFilterPtr> >::iterator it(filters.begin()), end(filters.end()); it != end; ++it)
		{
			IBaseFilterPtr& filter(static_cast<IBaseFilterPtr&>(*it));
			FILTER_INFO fi = {0};
			filter->QueryFilterInfo(&fi);
			IFilterGraphPtr ptr(fi.pGraph, false);

			append(new menu_entry(this, flt_id++, fi.achName));

			ISpecifyPropertyPagesPtr spp;
			filter->QueryInterface(&spp);

			if(!spp)
			{
				::EnableMenuItem(get_menu(), get_items().back()->get_id(), MF_GRAYED);
			}
		}
	}
	MENUITEMINFOW info = { sizeof(MENUITEMINFOW) };
	const UINT position(get_parent()->get_offset(this));
	::GetMenuItemInfoW(get_parent()->get_menu(), position, TRUE, &info);
	info.fMask = MIIM_SUBMENU;
	info.hSubMenu = get_menu();
	::SetMenuItemInfoW(get_parent()->get_menu(), position, TRUE, &info);
}

void awkawk_filter_menu::onUnInitMenuPopup(HWND, HMENU, WORD)
{
	if(get_menu() != 0)
	{
		while(get_items().size() > 0)
		{
			remove(0);
		}
	}
}

void awkawk_filter_menu::onCommand(HWND, int id, HWND, UINT)
{
	size_t chosen(id - filter_menu_base);
	std::vector<ATL::CAdapt<IBaseFilterPtr> > filters(player->get_filters());
	IBaseFilterPtr& filter(static_cast<IBaseFilterPtr&>(filters[chosen]));
	FILTER_INFO fi = {0};
	filter->QueryFilterInfo(&fi);
	IFilterGraphPtr ptr(fi.pGraph, false);

	ISpecifyPropertyPagesPtr spp;
	filter->QueryInterface(&spp);
	if(spp)
	{
		IUnknownPtr unk;
		filter->QueryInterface(&unk);
		IUnknown* unks[] = { unk.GetInterfacePtr() };
		CAUUID uuids;
		spp->GetPages(&uuids);
		ON_BLOCK_EXIT(::CoTaskMemFree(uuids.pElems));
		::OleCreatePropertyFrame(player->get_ui()->get_window(), 0, 0, fi.achName, 1, unks, uuids.cElems, uuids.pElems, 0, 0, nullptr);
	}
}

void awkawk_main_menu::onInitMenuPopup(HWND, HMENU, UINT, BOOL)
{
	bool play_menu_enabled(player->permitted(awkawk::play) || player->permitted(awkawk::pause) || player->permitted(awkawk::stop));
	::EnableMenuItem(get_menu(), IDM_OPEN_FILE, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_OPEN_URL, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_OPEN_URL, MF_GRAYED);
	::EnableMenuItem(get_menu(), 3, play_menu_enabled ? MF_ENABLED | MF_BYPOSITION : MF_GRAYED | MF_DISABLED | MF_BYPOSITION);
	::EnableMenuItem(get_menu(), 6, play_menu_enabled ? MF_ENABLED | MF_BYPOSITION : MF_GRAYED | MF_DISABLED | MF_BYPOSITION);
	::EnableMenuItem(get_menu(), IDM_CLOSE_FILE, play_menu_enabled ? MF_ENABLED : MF_GRAYED);
	::EnableMenuItem(get_menu(), IDM_EXIT, MF_ENABLED);
}
