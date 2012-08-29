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
#include "window_hook.h"

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

#define RW(prc) ((prc)->right-(prc)->left)
#define RH(prc) ((prc)->bottom-(prc)->top)

extern "C" { extern void _RTC_CheckEsp(void); }

struct edit_box_item : control, virtual menu_entry
{
	edit_box_item(window* owner, menu* parent, UINT id, const std::wstring& caption) : control(owner),
	                                                                                   menu_entry(parent, id, caption),
	                                                                                   menu_item(parent, id),
	                                                                                   created(::CreateEventW(nullptr, FALSE, FALSE, nullptr))
	{
	}

	enum
	{
		post_init_menu_popup = WM_APP + 1,
		set_focus_to_edit
	};

	virtual ~edit_box_item()
	{
		::CloseHandle(created);
	}

	virtual LRESULT CALLBACK message_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
	{
		switch(message)
		{
		case WM_INITMENUPOPUP:
			{
				if(reinterpret_cast<HMENU>(wParam) == get_parent()->get_menu())
				{
					handled = true;
					return HANDLE_WM_INITMENUPOPUP(window, wParam, lParam, onInitMenuPopup);
				}
			}
			break;
		case WM_UNINITMENUPOPUP:
			{
				if(reinterpret_cast<HMENU>(wParam) == get_parent()->get_menu())
				{
					handled = true;
					return HANDLE_WM_UNINITMENUPOPUP(window, wParam, lParam, onUnInitMenuPopup);
				}
			}
			break;
		case post_init_menu_popup:
			{
				if(reinterpret_cast<HMENU>(wParam) == get_parent()->get_menu())
				{
					handled = true;
					return HANDLE_WM_INITMENUPOPUP(window, wParam, lParam, onPostInitMenuPopup);
				}
			}
			break;
		case WM_MENUSELECT:
			{
				if(reinterpret_cast<HMENU>(lParam) == get_parent()->get_menu())
				{
					handled = true;
					return HANDLE_WM_MENUSELECT(window, wParam, lParam, onMenuSelect);
				}
			}
			break;
		}
		derr << "warning: we're claiming to handle a message but we're unable to dispatch it" << std::endl;
		return 0;
	}

	virtual void onInitMenuPopup(HWND wnd, HMENU menu, UINT, BOOL)
	{
		::PostMessage(wnd, post_init_menu_popup, reinterpret_cast<WPARAM>(menu), 0);
	}

	virtual void onPostInitMenuPopup(HWND, HMENU, UINT, BOOL)
	{
		hook_function = make_hook_proc(this, &edit_box_item::hook_proc);
		hook = ::SetWindowsHookExW(WH_GETMESSAGE, hook_function, NULL, ::GetCurrentThreadId());

		::GetMenuItemRect(NULL, get_parent()->get_menu(), get_parent()->get_offset(this), &edit_rect);
		edit_rect.left += ::GetSystemMetrics(SM_CXMENUCHECK);
		thread = utility::CreateThread(nullptr, 0, this, &edit_box_item::show_box, nullptr, 0, &thread_id);
		::WaitForSingleObject(created, INFINITE);
		::PostThreadMessage(thread_id, set_focus_to_edit, FALSE, 0);
	}

	virtual void onUnInitMenuPopup(HWND, HMENU, WORD)
	{
		::PostThreadMessageW(thread_id, WM_QUIT, 0, 0);
		::WaitForSingleObject(thread, INFINITE);
		::CloseHandle(thread);

		::UnhookWindowsHookEx(hook);
		destroy_hook_proc(hook_function);
	}

	virtual void onMenuSelect(HWND wnd, HMENU menu, UINT item, HMENU popup, UINT flags)
	{
		if(item == get_id())
		{
			::PostThreadMessageW(thread_id, set_focus_to_edit, TRUE, 0);
		}
		else
		{
			::PostThreadMessageW(thread_id, set_focus_to_edit, FALSE, 0);
		}
		FORWARD_WM_MENUSELECT(wnd, menu, item, popup, flags, ::DefWindowProcW);
	}

private:
	LRESULT CALLBACK hook_proc(int code, WPARAM wParam, LPARAM lParam)
	{
		if(code == HC_ACTION && wParam == PM_REMOVE)
		{
			MSG* msg(reinterpret_cast<MSG*>(lParam));
			if(::WindowFromPoint(msg->pt) == edit_control || edit_control == ::GetFocus() || msg->hwnd == edit_control)
			{
				switch(msg->message)
				{
				case WM_LBUTTONDOWN:
				case WM_LBUTTONUP:
				case WM_LBUTTONDBLCLK:
				case WM_RBUTTONDOWN:
				case WM_RBUTTONUP:
				case WM_RBUTTONDBLCLK:
					break;
				case WM_KEYDOWN:
				case WM_KEYUP:
				case WM_SYSKEYUP:
				case WM_SYSKEYDOWN:
					if(msg->wParam == VK_DOWN || msg->wParam == VK_UP || msg->wParam == VK_RETURN)
					{
						break;
					}
				case EM_GETSEL:
				case EM_SETSEL:
				case EM_GETRECT:
				case EM_SETRECT:
				case EM_SETRECTNP:
				case EM_SCROLL:
				case EM_LINESCROLL:
				case EM_SCROLLCARET:
				case EM_GETMODIFY:
				case EM_SETMODIFY:
				case EM_GETLINECOUNT:
				case EM_LINEINDEX:
				case EM_SETHANDLE:
				case EM_GETHANDLE:
				case EM_GETTHUMB:
				case EM_LINELENGTH:
				case EM_REPLACESEL:
				case EM_GETLINE:
				case EM_CANUNDO:
				case EM_UNDO:
				case EM_FMTLINES:
				case EM_LINEFROMCHAR:
				case EM_SETTABSTOPS:
				case EM_SETPASSWORDCHAR:
				case EM_EMPTYUNDOBUFFER:
				case EM_GETFIRSTVISIBLELINE:
				case EM_SETREADONLY:
				case EM_SETWORDBREAKPROC:
				case EM_GETWORDBREAKPROC:
				case EM_GETPASSWORDCHAR:
				case EM_SETMARGINS:
				case EM_GETMARGINS:
				case EM_SETLIMITTEXT:
				case EM_GETLIMITTEXT:
				case EM_POSFROMCHAR:
				case EM_CHARFROMPOS:
					::PostMessageW(edit_control, msg->message, msg->wParam, msg->lParam);
					msg->message = WM_NULL;
					msg->wParam = 0;
					msg->lParam = 0;
					return 0;
				}
			}
		}
		return ::CallNextHookEx(NULL, code, wParam, lParam);
	}

	DWORD show_box(void*)
	{
		edit_control = ::CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"EDIT", L"Lorem Ipsum", WS_POPUP | WS_BORDER, edit_rect.left, edit_rect.top, RW(&edit_rect), RH(&edit_rect), NULL, NULL, ::GetModuleHandle(NULL), nullptr);
		ON_BLOCK_EXIT(::DestroyWindow(edit_control));
		SetWindowFont(edit_control, GetStockFont(DEFAULT_GUI_FONT), FALSE);
		::SetWindowPos(edit_control, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
		Edit_SetSel(edit_control, 0, -1);

		MSG msg = {0};
		::PeekMessageW(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
		::SetEvent(created);

		while(::GetMessageW(&msg, NULL, 0, 0))
		{
			if(msg.message == set_focus_to_edit)
			{
				if(msg.wParam)
				{
					::SetFocus(edit_control);
				}
				else
				{
					::SetFocus(NULL);
				}
			}
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}

		::GetWindowTextW(edit_control, szText, ARRAYSIZE(szText));
		return 0;
	}

	HANDLE created;
	HANDLE thread;
	DWORD thread_id;
	HWND edit_control;
	RECT edit_rect;
	wchar_t szText[MAX_PATH];
	HHOOK hook;
	HOOKPROC hook_function;
	std::wstring value;
};

struct custom_drawn_item : control, virtual menu_item
{
	custom_drawn_item(window* owner, menu* parent, UINT id) : control(owner), menu_item(parent, id)
	{
		get_item_info().fMask |= MIIM_FTYPE;
		get_item_info().fType |= MFT_OWNERDRAW;
	}

	virtual bool handles_message(HWND, UINT message, WPARAM, LPARAM lParam) const
	{
		switch(message)
		{
		case WM_MEASUREITEM:
			{
				MEASUREITEMSTRUCT* mis(reinterpret_cast<MEASUREITEMSTRUCT*>(lParam));
				return mis->CtlType == ODT_MENU && mis->itemID == get_id();
			}
			break;
		case WM_DRAWITEM:
			{
				const DRAWITEMSTRUCT* dis(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
				return dis->CtlType == ODT_MENU && dis->itemID == get_id();
			}
			break;
		}
		return false;
	}

	virtual LRESULT CALLBACK message_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
	{
		UNREFERENCED_PARAMETER(wParam);
		switch(message)
		{
		case WM_MEASUREITEM:
			{
				MEASUREITEMSTRUCT* mis(reinterpret_cast<MEASUREITEMSTRUCT*>(lParam));
				if(mis->CtlType == ODT_MENU && mis->itemID == get_id())
				{
					handled = true;
					return HANDLE_WM_MEASUREITEM(window, wParam, lParam, onMeasureItem);
				}
			}
		case WM_DRAWITEM:
			{
				const DRAWITEMSTRUCT* dis(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
				if(dis->CtlType == ODT_MENU && dis->itemID == get_id())
				{
					handled = true;
					return HANDLE_WM_DRAWITEM(window, wParam, lParam, onDrawItem);
				}
			}
		}
		return 0;
	}

	virtual void onMeasureItem(HWND window, MEASUREITEMSTRUCT* measure_item) = 0;
	virtual void onDrawItem(HWND window, const DRAWITEMSTRUCT* draw_item) = 0;
};

struct custom_drawn_separator_item : custom_drawn_item, menu_separator
{
	custom_drawn_separator_item(window* owner, menu* parent, UINT id) : custom_drawn_item(owner, parent, id), menu_separator(parent), menu_item(parent, id)
	{
	}
};

struct custom_drawn_string_item : custom_drawn_item, menu_entry
{
	custom_drawn_string_item(window* owner, menu* parent, UINT id, const std::wstring& caption_) : custom_drawn_item(owner, parent, id), menu_entry(parent, id, caption_), menu_item(parent, id)
	{
	}
};

struct theme_metrics
{
	theme_metrics(HWND window_) : window(window_), theme(::OpenThemeData(window, VSCLASS_MENU))
	{
		::GetThemePartSize(theme, NULL, MENU_POPUPCHECK, 0, nullptr, TS_TRUE, &popup_checkbox_size);
		::GetThemePartSize(theme, NULL, MENU_POPUPSEPARATOR, 0, nullptr, TS_TRUE, &popup_separator);

		::GetThemeInt(theme, MENU_POPUPITEM, 0, TMT_BORDERSIZE, &popup_border);
		::GetThemeInt(theme, MENU_POPUPBACKGROUND, 0, TMT_BORDERSIZE, &popup_gutter);

		::GetThemeMargins(theme, NULL, MENU_POPUPCHECK, 0, TMT_CONTENTMARGINS, nullptr, &popup_checkbox_margins);
		::GetThemeMargins(theme, NULL, MENU_POPUPCHECKBACKGROUND, 0, TMT_CONTENTMARGINS, nullptr, &popup_checkbox_background);
		::GetThemeMargins(theme, NULL, MENU_POPUPITEM, 0, TMT_CONTENTMARGINS, nullptr, &popup_item);

		popup_accelerator = popup_item;
		popup_accelerator.cxLeftWidth = popup_accelerator.cxRightWidth = 0;

		MARGINS margins = popup_item;
		margins.cxRightWidth = popup_border;
		margins.cxLeftWidth = popup_gutter;
		popup_text = margins;
		
		popup_checkbox_gutter = popup_checkbox_background.cyTopHeight + popup_checkbox_background.cyBottomHeight;
	}

	~theme_metrics()
	{
		::CloseThemeData(theme);
	}

	POPUPITEMSTATES to_popup_item_state(UINT item_state)
	{
		const int disabled((item_state & (ODS_INACTIVE | ODS_DISABLED)) != 0);
		const int hot((item_state & (ODS_HOTLIGHT | ODS_SELECTED)) != 0);
		switch((disabled << 1) | hot)
		{
		default:
		case 0:
			return MPI_NORMAL;
		case 1:
			return MPI_HOT;
		case 2:
			return MPI_DISABLED;
		case 3:
			return MPI_DISABLEDHOT;
		}
	}

	POPUPCHECKBACKGROUNDSTATES to_popup_check_background_state(int state)
	{
		if(state == MPI_DISABLED || state == MPI_DISABLEDHOT)
		{
			return MCB_DISABLED;
		}
		return MCB_NORMAL;
	}

	POPUPCHECKSTATES to_popup_check_state(UINT type, int state)
	{
		if(type & MFT_RADIOCHECK)
		{
			if(state == MPI_DISABLED || state == MPI_DISABLEDHOT)
			{
				return MC_BULLETDISABLED;
			}
			else
			{
				return MC_BULLETNORMAL;
			}
		}
		else
		{
			if(state == MPI_DISABLED || state == MPI_DISABLEDHOT)
			{
				return MC_CHECKMARKDISABLED;
			}
			else
			{
				return MC_CHECKMARKNORMAL;
			}
		}
	}

	void to_measure_size(const SIZE* psizeDraw, const MARGINS* pmargins, __out SIZE* psizeMeasure)
	{
		psizeMeasure->cx = psizeDraw->cx + pmargins->cxLeftWidth + pmargins->cxRightWidth;
		psizeMeasure->cy = psizeDraw->cy + pmargins->cyTopHeight + pmargins->cyBottomHeight;
	}

	void to_draw_rect(const RECT* prcMeasure, const MARGINS* pmargins, __out RECT* prcDraw)
	{
		// Convert the measure rect to a drawing rect
		::SetRect(prcDraw, prcMeasure->left   + pmargins->cxLeftWidth,
		                   prcMeasure->top    + pmargins->cyTopHeight,
		                   prcMeasure->right  - pmargins->cxRightWidth,
		                   prcMeasure->bottom - pmargins->cyBottomHeight);
	}

	MARGINS popup_checkbox_margins;
	MARGINS popup_checkbox_background;
	MARGINS popup_item;
	MARGINS popup_text;
	MARGINS popup_accelerator;

	SIZE popup_checkbox_size;
	SIZE popup_separator;
	int popup_border;
	int popup_gutter;
	int popup_checkbox_gutter;

	HWND window;
	HTHEME theme;
};

struct themed_item
{
	enum menu_parts
	{
		menu_checkbox,
		menu_text,
		menu_separator,
		menu_parts_max
	};

	struct draw_item_metrics
	{
		RECT selection; // Selection rectangle
		RECT gutter; // Gutter rectangle
		RECT check_background; // Check background rectangle
		RECT parts[menu_parts_max]; // Menu subitem rectangles
	};

	virtual theme_metrics& get_metrics()
	{
		if(metrics.get() == nullptr)
		{
			// sigh.  I disable theming on the main window because I want square corners,
			// but this means I no longer know what theme it has loaded.  I hope the desktop
			// window has the right theme, but I don't think it's under any particular obligation to do so.
			metrics.reset(new theme_metrics(::GetDesktopWindow()));
			//metrics.reset(new theme_metrics(get_owning_window()->get_window()));
		}
		return *metrics;
	}

	std::unique_ptr<theme_metrics> metrics;
	SIZE part_sizes[menu_parts_max];
};

struct themed_separator_item : themed_item, custom_drawn_separator_item
{
	themed_separator_item(window* owner, menu* parent, UINT id) : custom_drawn_separator_item(owner, parent, id), menu_item(parent, id)
	{
	}

	virtual void onMeasureItem(HWND window, MEASUREITEMSTRUCT* measure_item);
	virtual void onDrawItem(HWND window, const DRAWITEMSTRUCT* draw_item);
};

struct themed_string_item : themed_item, custom_drawn_string_item
{
	themed_string_item(window* owner, menu* parent, UINT id, const std::wstring& caption) : custom_drawn_string_item(owner, parent, id, caption), menu_item(parent, id)
	{
	}

	virtual void onMeasureItem(HWND window, MEASUREITEMSTRUCT* measure_item);
	virtual void onDrawItem(HWND window, const DRAWITEMSTRUCT* draw_item);
};

#endif
