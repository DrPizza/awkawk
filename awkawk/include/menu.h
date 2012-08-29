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

#ifndef MENU__H
#define MENU__H

#include "window.h"

struct menu;

struct menu_item
{
	menu_item(menu* parent_, UINT id_) : parent(parent_), id(id_)
	{
		init();
	}

	menu_item(menu* parent_) : parent(parent_), id()
	{
		init();
	}

	virtual ~menu_item()
	{
	}

	const MENUITEMINFOW& get_item_info() const
	{
		return menu_item_info;
	}

	MENUITEMINFOW& get_item_info()
	{
		return menu_item_info;
	}

	UINT get_id() const
	{
		return id;
	}

	const menu* get_parent() const
	{
		return parent;
	}

	menu* get_parent()
	{
		return parent;
	}

protected:
	menu_item(UINT id_) : parent(), id(id_)
	{
		init();
	}

	menu_item() : parent(), id()
	{
		init();
	}

private:
	void init()
	{
		::memset(&menu_item_info, 0, sizeof(menu_item_info));
		menu_item_info.cbSize = sizeof(menu_item_info);
		if(id != 0)
		{
			get_item_info().fMask |= MIIM_ID;
			get_item_info().wID = id;
		}
	}
	menu* parent;
	UINT id;
	MENUITEMINFOW menu_item_info;
};

struct menu_entry : virtual menu_item
{
	menu_entry(menu* parent, UINT id, const std::wstring& caption_) : menu_item(parent, id), caption(caption_), bitmap(NULL)
	{
		init();
	}

	menu_entry(menu* parent, UINT id, HBITMAP bitmap_) : menu_item(parent, id), caption(), bitmap(bitmap_)
	{
		init();
	}

	menu_entry(menu* parent, const std::wstring& caption_) : menu_item(parent), caption(caption_), bitmap(NULL)
	{
		init();
	}

	menu_entry(menu* parent, HBITMAP bitmap_) : menu_item(parent), caption(), bitmap(bitmap_)
	{
		init();
	}

	const std::wstring& get_caption() const
	{
		return caption;
	}

	HBITMAP get_bitmap() const
	{
		return bitmap;
	}

protected:
	menu_entry(UINT id, const std::wstring& caption_) : menu_item(id), caption(caption_), bitmap(NULL)
	{
		init();
	}

	menu_entry(UINT id, HBITMAP bitmap_ = NULL) : menu_item(id), caption(), bitmap(bitmap_)
	{
		init();
	}

	menu_entry(const std::wstring& caption_) : menu_item(), caption(caption_), bitmap(NULL)
	{
		init();
	}

	menu_entry(HBITMAP bitmap_) : menu_item(), caption(), bitmap(bitmap_)
	{
		init();
	}

private:
	void init()
	{
		if(!caption.empty())
		{
			get_item_info().fMask |= MIIM_STRING;
			get_item_info().fType |= MFT_STRING;
			get_item_info().cch = static_cast<UINT>(caption.size());
			get_item_info().dwTypeData = const_cast<wchar_t*>(caption.c_str());
		}
		if(bitmap != NULL)
		{
			get_item_info().fMask |= MIIM_BITMAP;
			get_item_info().fType |= MFT_BITMAP;
			get_item_info().hbmpItem = bitmap;
		}
	}

	std::wstring caption;
	HBITMAP bitmap;
};

struct menu_separator : virtual menu_item
{
	menu_separator(menu* parent) : menu_item(parent)
	{
		get_item_info().fMask |= MIIM_FTYPE;
		get_item_info().fType |= MFT_SEPARATOR;
	}
};

struct menu : control, menu_entry
{
	menu(window* owner, HMENU handle_, menu* parent, UINT id = 0, const std::wstring& caption = L"")
		: control(owner), menu_entry(parent, id, caption), handle(handle_), menu_item(parent, id)
	{
		init();
	}

	menu(window* owner, HMENU handle_, menu* parent, UINT id = 0, HBITMAP bitmap = NULL)
		: control(owner), menu_entry(parent, id, bitmap), handle(handle_), menu_item(parent, id)
	{
		init();
	}

	menu(window* owner, menu* parent, UINT id = 0, const std::wstring& caption = L"")
		: control(owner), menu_entry(parent, id, caption), handle(::CreatePopupMenu()), menu_item(parent, id)
	{
		init();
	}

	menu(window* owner, menu* parent, UINT id = 0, HBITMAP bitmap = NULL)
		: control(owner), menu_entry(parent, id, bitmap), handle(::CreatePopupMenu()), menu_item(parent, id)
	{
		init();
	}

	menu(window* owner, HMENU handle_, UINT id = 0, const std::wstring& caption = L"")
		: control(owner), menu_entry(id, caption), handle(handle_), menu_item(id)
	{
		init();
	}

	menu(window* owner, HMENU handle_, UINT id = 0, HBITMAP bitmap = NULL)
		: control(owner), menu_entry(id, bitmap), handle(handle_), menu_item(id)
	{
		init();
	}

	menu(window* owner, UINT id = 0, const std::wstring& caption = L"")
		: control(owner), menu_entry(id, caption), handle(::CreatePopupMenu()), menu_item(id)
	{
		init();
	}

	menu(window* owner, UINT id = 0, HBITMAP bitmap = NULL)
		: control(owner), menu_entry(id, bitmap), handle(::CreatePopupMenu()), menu_item(id)
	{
		init();
	}

	~menu()
	{
		if(::IsMenu(get_menu()))
		{
			::DestroyMenu(get_menu());
		}
	}

	BOOL TrackPopupMenu(UINT flags, int x, int y, int reserved, HWND owner, const RECT* src)
	{
		return ::TrackPopupMenu(get_menu(), flags, x, y, reserved, owner, src);
	}

	BOOL TrackPopupMenuEx(UINT flags, int x, int y, HWND owner, LPTPMPARAMS params)
	{
		return ::TrackPopupMenuEx(get_menu(), flags, x, y, owner, params);
	}

	MENUINFO get_info() const
	{
		MENUINFO mi = { sizeof(MENUINFO) };
		return mi;
	}

	void append(menu_item* new_item)
	{
		items.push_back(std::shared_ptr<menu_item>(new_item));
		MENUITEMINFOW mii(new_item->get_item_info());
		::InsertMenuItemW(get_menu(), static_cast<UINT>(items.size() - 1), TRUE, &mii);
	}

	void insert(menu_item* new_item, UINT position)
	{
		items.insert(items.begin() + position, std::shared_ptr<menu_item>(new_item));
		MENUITEMINFOW mii(new_item->get_item_info());
		::InsertMenuItemW(get_menu(), position, TRUE, &mii);
	}

	void remove(UINT position)
	{
		::DeleteMenu(get_menu(), position, MF_BYPOSITION);
		items.erase(items.begin() + position);
	}

	virtual LRESULT CALLBACK message_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
	{
		switch(message)
		{
		case WM_INITMENU:
			{
				if(reinterpret_cast<HMENU>(wParam) == get_menu())
				{
					handled = true;
					return HANDLE_WM_INITMENU(window, wParam, lParam, onInitMenu);
				}
			}
			break;
		case WM_INITMENUPOPUP:
			{
				if(reinterpret_cast<HMENU>(wParam) == get_menu())
				{
					handled = true;
					return HANDLE_WM_INITMENUPOPUP(window, wParam, lParam, onInitMenuPopup);
				}
			}
			break;
		case WM_UNINITMENUPOPUP:
			{
				if(reinterpret_cast<HMENU>(wParam) == get_menu())
				{
					handled = true;
					return HANDLE_WM_UNINITMENUPOPUP(window, wParam, lParam, onUnInitMenuPopup);
				}
			}
			break;
		case WM_MENUCOMMAND:
			{
				if(reinterpret_cast<HMENU>(lParam) == get_menu() && static_cast<UINT>(wParam) < items.size())
				{
					handled = true;
					dout << "got a hit by position.  Even though I don't think we've asked for it..." << std::endl;
					return HANDLE_WM_MENUCOMMAND(window, wParam, lParam, onMenuCommand);
				}
			}
			break;
		case WM_ENTERMENULOOP:
			{
				return HANDLE_WM_ENTERMENULOOP(window, wParam, lParam, onEnterMenuLoop);
			}
			break;
		case WM_EXITMENULOOP:
			{
				return HANDLE_WM_EXITMENULOOP(window, wParam, lParam, onExitMenuLoop);
			}
			break;
		case WM_MENUSELECT:
			{
				if(reinterpret_cast<HMENU>(lParam) == get_menu())
				{
					handled = true;
					return HANDLE_WM_MENUSELECT(window, wParam, lParam, onMenuSelect);
				}
			}
			break;
		}
		return 0;
	}

	virtual void onInitMenu(HWND wnd, HMENU menu)
	{
		FORWARD_WM_INITMENU(wnd, menu, ::DefWindowProcW);
	}

	virtual void onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu)
	{
		FORWARD_WM_INITMENUPOPUP(wnd, menu, item, window_menu, ::DefWindowProcW);
	}

	virtual void onUnInitMenuPopup(HWND wnd, HMENU menu, WORD type)
	{
		FORWARD_WM_UNINITMENUPOPUP(wnd, menu, type, ::DefWindowProcW);
	}

	virtual void onMenuCommand(HWND wnd, HMENU menu, UINT position)
	{
		FORWARD_WM_MENUCOMMAND(wnd, menu, position, ::DefWindowProcW);
	}

	virtual void onEnterMenuLoop(HWND wnd, BOOL popup)
	{
		FORWARD_WM_ENTERMENULOOP(wnd, popup, ::DefWindowProcW);
	}

	virtual void onExitMenuLoop(HWND wnd, BOOL popup)
	{
		FORWARD_WM_EXITMENULOOP(wnd, popup, ::DefWindowProcW);
	}

	virtual void onMenuSelect(HWND wnd, HMENU menu, int item, HMENU popup, UINT flags)
	{
		FORWARD_WM_MENUSELECT(wnd, menu, item, popup, flags, ::DefWindowProcW);
	}

	HMENU get_menu() const
	{
		return handle;
	}

	UINT get_offset(const menu_item* itm) const
	{
		for(UINT os(0); os < items.size(); ++os)
		{
			if(items[os].get() == itm)
			{
				return os;
			}
		}
		return static_cast<UINT>(-1);
	}

protected:
	void set_menu(HMENU hnd)
	{
		handle = hnd;
	}

	const std::vector<std::shared_ptr<menu_item> > get_items() const
	{
		return items;
	}

private:
	void init()
	{
		get_item_info().fMask |= MIIM_SUBMENU;
		get_item_info().hSubMenu = get_menu();
	}

	HMENU handle;
	std::vector<std::shared_ptr<menu_item> > items;
};

#endif
