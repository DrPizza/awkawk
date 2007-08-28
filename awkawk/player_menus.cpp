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

awkawk_menu::awkawk_menu(awkawk* player_, window* owner, const wchar_t* caption) : menu(owner, caption), player(player_)
{
}

awkawk_menu::awkawk_menu(awkawk* player_, window* owner, const wchar_t* caption, menu* parent) : menu(owner, caption, parent), player(player_)
{
}

void awkawk_play_menu::onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu)
{
	::EnableMenuItem(get_menu(), IDM_PLAY, player->permitted(awkawk::play) ? MF_ENABLED : MF_GRAYED);
	::EnableMenuItem(get_menu(), IDM_PAUSE, player->permitted(awkawk::pause) ? MF_ENABLED : MF_GRAYED);
	::EnableMenuItem(get_menu(), IDM_STOP, player->permitted(awkawk::stop) ? MF_ENABLED : MF_GRAYED);
}

void awkawk_play_menu::onCommand(HWND wnd, int id, HWND control, UINT event)
{
	switch(id)
	{
	case IDM_PAUSE:
		player->post_event(awkawk::pause);
		break;
	case IDM_PLAY:
		player->post_event(awkawk::play);
		break;
	case IDM_STOP:
		player->post_event(awkawk::stop);
		break;
	}
}

void awkawk_mode_menu::onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu)
{
	::EnableMenuItem(get_menu(), IDM_PLAYMODE_NORMAL, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_PLAYMODE_REPEATALL, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_PLAYMODE_REPEATTRACK, MF_ENABLED);
	//::EnableMenuItem(playmode_menu, IDM_PLAYMODE_SHUFFLE, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_PLAYMODE_SHUFFLE, MF_GRAYED);
	::CheckMenuRadioItem(get_menu(), IDM_PLAYMODE_NORMAL, IDM_PLAYMODE_SHUFFLE, player->get_playmode(), MF_BYCOMMAND);
}

void awkawk_mode_menu::onCommand(HWND wnd, int id, HWND control, UINT event)
{
	switch(id)
	{
	case IDM_PLAYMODE_NORMAL:
		player->set_playmode(awkawk::normal);
		break;
	case IDM_PLAYMODE_REPEATALL:
		player->set_playmode(awkawk::repeat_all);
		break;
	case IDM_PLAYMODE_REPEATTRACK:
		player->set_playmode(awkawk::repeat_single);
		break;
	case IDM_PLAYMODE_SHUFFLE:
		player->set_playmode(awkawk::shuffle);
		break;
	}
}

void awkawk_size_menu::onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu)
{
	::EnableMenuItem(get_menu(), IDM_SIZE_50, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_SIZE_100, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_SIZE_200, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_SIZE_FREE, MF_ENABLED);
	::CheckMenuRadioItem(get_menu(), IDM_SIZE_50, IDM_SIZE_FREE, player->get_window_size_mode(), MF_BYCOMMAND);

	::EnableMenuItem(get_menu(), IDM_AR_ORIGINAL, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_AR_133TO1, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_AR_155TO1, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_AR_177TO1, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_AR_185TO1, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_AR_240TO1, MF_ENABLED);
	::CheckMenuItem(get_menu(), IDM_AR_240TO1, MF_CHECKED);
	::CheckMenuRadioItem(get_menu(), IDM_AR_ORIGINAL, IDM_AR_240TO1, player->get_aspect_ratio_mode(), MF_BYCOMMAND);
	::EnableMenuItem(get_menu(), IDM_AR_CUSTOM, MF_GRAYED);

	::EnableMenuItem(get_menu(), IDM_NOLETTERBOXING, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_4_TO_3_ORIGINAL, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_14_TO_9_ORIGINAL, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_16_TO_9_ORIGINAL, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_185_TO_1_ORIGINAL, MF_ENABLED);
	::EnableMenuItem(get_menu(), IDM_240_TO_1_ORIGINAL, MF_ENABLED);
	::CheckMenuRadioItem(get_menu(), IDM_NOLETTERBOXING, IDM_240_TO_1_ORIGINAL, player->get_letterbox_mode(), MF_BYCOMMAND);
}

void awkawk_size_menu::onCommand(HWND wnd, int id, HWND control, UINT event)
{
	switch(id)
	{
	case IDM_SIZE_50:
		player->set_window_size_mode(awkawk::fifty_percent);
		break;
	case IDM_SIZE_100:
		player->set_window_size_mode(awkawk::one_hundred_percent);
		break;
	case IDM_SIZE_200:
		player->set_window_size_mode(awkawk::two_hundred_percent);
		break;
	case IDM_SIZE_FREE:
		player->set_window_size_mode(awkawk::free);
		break;
	case IDM_AR_ORIGINAL:
		player->set_aspect_ratio_mode(awkawk::original);
		break;
	case IDM_AR_133TO1:
		player->set_aspect_ratio_mode(awkawk::onethreethree_to_one);
		break;
	case IDM_AR_155TO1:
		player->set_aspect_ratio_mode(awkawk::onefivefive_to_one);
		break;
	case IDM_AR_177TO1:
		player->set_aspect_ratio_mode(awkawk::onesevenseven_to_one);
		break;
	case IDM_AR_185TO1:
		player->set_aspect_ratio_mode(awkawk::oneeightfive_to_one);
		break;
	case IDM_AR_240TO1:
		player->set_aspect_ratio_mode(awkawk::twofourzero_to_one);
		break;
	case IDM_NOLETTERBOXING:
		player->set_letterbox_mode(awkawk::no_letterboxing);
		break;
	case IDM_4_TO_3_ORIGINAL:
		player->set_letterbox_mode(awkawk::four_to_three_original);
		break;
	case IDM_14_TO_9_ORIGINAL:
		player->set_letterbox_mode(awkawk::fourteen_to_nine_original);
		break;
	case IDM_16_TO_9_ORIGINAL:
		player->set_letterbox_mode(awkawk::sixteen_to_nine_original);
		break;
	case IDM_185_TO_1_ORIGINAL:
		player->set_letterbox_mode(awkawk::oneeightfive_to_one_original);
		break;
	case IDM_240_TO_1_ORIGINAL:
		player->set_letterbox_mode(awkawk::twofourzero_to_one_original);
		break;
	}
}

void awkawk_filter_menu::build_filter_menu(UINT position)
{
	if(get_menu() != 0)
	{
		int count(::GetMenuItemCount(get_menu()));
		for(int i(0); i < count; ++i)
		{
			::DeleteMenu(get_menu(), filter_menu_base + i, MF_BYCOMMAND);
		}
		::DestroyMenu(get_menu());
		set_menu(0);
	}
	if(player->permitted(awkawk::play) || player->permitted(awkawk::pause) || player->permitted(awkawk::stop))
	{
		set_menu(::CreatePopupMenu());

		std::vector<CAdapt<IBaseFilterPtr> > filters(player->get_filters());
		UINT flt_id(filter_menu_base);
		for(std::vector<CAdapt<IBaseFilterPtr> >::iterator it(filters.begin()), end(filters.end()); it != end; ++it)
		{
			IBaseFilterPtr& filter(static_cast<IBaseFilterPtr&>(*it));
			FILTER_INFO fi = {0};
			filter->QueryFilterInfo(&fi);
			IFilterGraphPtr ptr(fi.pGraph, false);
			
			ISpecifyPropertyPagesPtr spp;
			filter->QueryInterface(&spp);
			DWORD state(MFS_ENABLED);
			if(!spp)
			{
				state = MFS_DISABLED;
			}
			
			MENUITEMINFOW info = { sizeof(MENUITEMINFOW) };
			info.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
			info.wID = flt_id;
			info.dwTypeData = fi.achName;
			info.fState = state;
			::InsertMenuItemW(get_menu(), flt_id++, FALSE, &info);
		}
	}
	MENUITEMINFOW info = { sizeof(MENUITEMINFOW) };
	::GetMenuItemInfoW(get_parent()->get_menu(), position, TRUE, &info);
	info.fMask = MIIM_SUBMENU;
	info.hSubMenu = get_menu();
	::SetMenuItemInfoW(get_parent()->get_menu(), position, TRUE, &info);
}

bool awkawk_filter_menu::show_filter_properties(size_t chosen) const
{
	std::vector<CAdapt<IBaseFilterPtr> > filters(player->get_filters());
	if(chosen < filters.size())
	{
		FILTER_INFO fi = {0};
		IBaseFilterPtr& filter(static_cast<IBaseFilterPtr&>(filters[chosen]));
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
			ON_BLOCK_EXIT(&CoTaskMemFree, uuids.pElems);
			::OleCreatePropertyFrame(player->get_ui()->get_window(), 0, 0, fi.achName, 1, unks, uuids.cElems, uuids.pElems, 0, 0, NULL);
			return true;
		}
	}
	return false;
}

void awkawk_filter_menu::onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu)
{
	build_filter_menu(get_parent()->get_offset(this));
}

void awkawk_filter_menu::onCommand(HWND wnd, int id, HWND control, UINT event)
{
	if(!show_filter_properties(id - (filter_menu_base)))
	{
		FORWARD_WM_COMMAND(wnd, id, control, event, ::DefWindowProcW);
	}
}

void awkawk_main_menu::onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu)
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

void awkawk_main_menu::onCommand(HWND wnd, int id, HWND control, UINT event)
{
	switch(id)
	{
	case IDM_OPEN_FILE:
	case IDM_OPEN_URL:
		{
			player->get_ui()->open_single_file(id == IDM_OPEN_FILE ? player->get_ui()->get_local_path() : player->get_ui()->get_remote_path());
		}
		break;
	case IDM_CLOSE_FILE:
		{
			player->get_ui()->set_window_text(player->get_ui()->get_app_title().c_str());
			player->send_event(awkawk::stop);
			player->send_event(awkawk::unload);
			player->clear_files();
		}
		break;
	case IDM_EXIT:
		player->get_ui()->destroy_window();
		break;
	}
}
