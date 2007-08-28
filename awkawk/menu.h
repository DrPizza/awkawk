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

#ifndef MENU__H
#define MENU__H

#pragma once

#include "window.h"

struct menu_item
{
	menu_item() : id()
	{
	}

	menu_item(UINT id_) : id(id_)
	{
	}

	virtual ~menu_item()
	{
	}

	virtual MENUITEMINFOW get_item_info() const
	{
		MENUITEMINFOW mii = { sizeof(MENUITEMINFOW) };
		if(id != 0)
		{
			mii.fMask = MIIM_ID;
			mii.wID = id;
		}
		return mii;
	}

	UINT get_id() const
	{
		return id;
	}

private:
	UINT id;
};

struct menu_separator : menu_item
{
	MENUITEMINFOW get_item_info() const
	{
		MENUITEMINFOW mii(menu_item::get_item_info());
		mii.fMask |= MIIM_FTYPE;
		mii.fType |= MFT_SEPARATOR;
		return mii;
	}
};

struct menu_string : menu_item
{
	menu_string(std::wstring caption_) : caption(caption_)
	{
	}

	menu_string(UINT id, std::wstring caption_) : menu_item(id), caption(caption_)
	{
	}

	MENUITEMINFOW get_item_info() const
	{
		MENUITEMINFOW mii(menu_item::get_item_info());
		mii.fMask |= MIIM_FTYPE | MIIM_STRING;
		mii.fType |= MFT_STRING;
		mii.cch = static_cast<UINT>(caption.size());
		mii.dwTypeData = const_cast<wchar_t*>(caption.c_str());
		return mii;
	}

	std::wstring caption;
};

struct menu_bitmap : menu_item
{
	menu_bitmap(HBITMAP bitmap_) : bitmap(bitmap_)
	{
	}

	menu_bitmap(UINT id, HBITMAP bitmap_) : menu_item(id), bitmap(bitmap_)
	{
	}

	MENUITEMINFOW get_item_info() const
	{
		MENUITEMINFOW mii(menu_item::get_item_info());
		mii.fMask |= MIIM_FTYPE | MIIM_BITMAP;
		mii.fType |= MFT_BITMAP;
		mii.dwTypeData = reinterpret_cast<wchar_t*>(bitmap);
		return mii;
	}

	HBITMAP bitmap;
};

struct menu : menu_string, control
{
	menu(window* owner, const wchar_t* caption) : handle(::CreatePopupMenu()), control(owner), menu_string(caption), parent(NULL)
	{
	}

	menu(window* owner, const wchar_t* caption, menu* parent_) : handle(::CreatePopupMenu()), control(owner), menu_string(caption), parent(parent_)
	{
	}

	menu(window* owner, const wchar_t* caption, menu* parent_, HMENU handle_) : handle(handle_), control(owner), menu_string(caption), parent(parent_)
	{
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

	MENUITEMINFOW get_item_info() const
	{
		MENUITEMINFOW mii(menu_string::get_item_info());
		mii.fMask |= MIIM_FTYPE | MIIM_SUBMENU;
		mii.hSubMenu = get_menu();
		return mii;
	}

	MENUINFO get_info() const
	{
		MENUINFO mi = { sizeof(MENUINFO) };
		return mi;
	}

	void construct_menu()
	{
		for(size_t i(0); i < items.size(); ++i)
		{
			MENUITEMINFOW mii(items[i]->get_item_info());
			::InsertMenuItemW(get_menu(), static_cast<UINT>(i), TRUE, &mii);
		}
	}

	void append(menu_item* new_item)
	{
		items.push_back(boost::shared_ptr<menu_item>(new_item));
	}

	virtual bool handles_message(HWND window, UINT message, WPARAM wParam, LPARAM lParam) const
	{
		switch(message)
		{
		case WM_INITMENU:
		case WM_INITMENUPOPUP:
			{
				if(reinterpret_cast<HMENU>(wParam) == get_menu())
				{
					return true;
				}
			}
			break;
		case WM_COMMAND:
			{
				if(HIWORD(wParam) == 0 || HIWORD(wParam) == 1)
				{
					for(std::vector<boost::shared_ptr<menu_item> >::const_iterator it(items.begin()), end(items.end()); it != end; ++it)
					{
						if((*it)->get_id() == LOWORD(wParam))
						{
							return true;
						}
					}
				}
			}
			break;
		case WM_MENUCOMMAND:
			{
				if(reinterpret_cast<HMENU>(lParam) == get_menu() && static_cast<UINT>(wParam) < items.size())
				{
					dout << "got a hit by position.  Even though I don't think we've asked for it..." << std::endl;
					return true;
				}
			}
			break;
		}
		return false;
	}

	virtual LRESULT CALLBACK message_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)
		{
		case WM_INITMENU:
			{
				if(reinterpret_cast<HMENU>(wParam) == get_menu())
				{
					return HANDLE_WM_INITMENU(window, wParam, lParam, onInitMenu);
				}
			}
			break;
		case WM_INITMENUPOPUP:
			{
				if(reinterpret_cast<HMENU>(wParam) == get_menu())
				{
					return HANDLE_WM_INITMENUPOPUP(window, wParam, lParam, onInitMenuPopup);
				}
			}
			break;
		case WM_COMMAND:
			{
				if(HIWORD(wParam) == 0 || HIWORD(wParam) == 1)
				{
					for(std::vector<boost::shared_ptr<menu_item> >::const_iterator it(items.begin()), end(items.end()); it != end; ++it)
					{
						if((*it)->get_id() == LOWORD(wParam))
						{
							return HANDLE_WM_COMMAND(window, wParam, lParam, onCommand);
						}
					}
				}
			}
			break;
		case WM_MENUCOMMAND:
			{
				if(reinterpret_cast<HMENU>(lParam) == get_menu() && static_cast<UINT>(wParam) < items.size())
				{
					dout << "got a hit by position.  Even though I don't think we've asked for it..." << std::endl;
					return HANDLE_WM_MENUCOMMAND(window, wParam, lParam, onMenuCommand);
				}
			}
			break;
		}
		derr << "warning: we're claiming to handle a message but we're unable to dispatch it" << std::endl;
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

	virtual void onCommand(HWND wnd, int id, HWND control, UINT event)
	{
		FORWARD_WM_COMMAND(wnd, id, control, event, ::DefWindowProcW);
	}

	virtual void onMenuCommand(HWND wnd, HMENU menu, UINT position)
	{
		FORWARD_WM_MENUCOMMAND(wnd, menu, position, ::DefWindowProcW);
	}

	HMENU get_menu() const
	{
		return handle;
	}

	const menu* get_parent() const
	{
		return parent;
	}

	menu* get_parent()
	{
		return parent;
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

private:
	std::vector<boost::shared_ptr<menu_item> > items;
	HMENU handle;
	menu* parent;
};

struct custom_drawn_item : control, menu_item
{
	custom_drawn_item(window* owner, UINT id, menu* parent_) : control(owner), menu_item(id), parent(parent_)
	{
	}

	virtual bool handles_message(HWND window, UINT message, WPARAM wParam, LPARAM lParam) const
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

	virtual LRESULT CALLBACK message_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)
		{
		case WM_MEASUREITEM:
			return HANDLE_WM_MEASUREITEM(window, wParam, lParam, onMeasureItem);
		case WM_DRAWITEM:
			return HANDLE_WM_DRAWITEM(window, wParam, lParam, onDrawItem);
		}
		derr << "warning: we're claiming to handle a message but we're unable to dispatch it" << std::endl;
		return 0;
	}

	virtual void onMeasureItem(HWND window, MEASUREITEMSTRUCT* measure_item) = 0;
	virtual void onDrawItem(HWND window, const DRAWITEMSTRUCT* draw_item) = 0;

	MENUITEMINFOW get_item_info() const
	{
		MENUITEMINFOW mii(menu_item::get_item_info());
		mii.fMask |= MIIM_FTYPE;
		mii.fType |= MFT_OWNERDRAW;
		return mii;
	}

	const menu* get_parent() const
	{
		return parent;
	}

	menu* get_parent()
	{
		return parent;
	}

private:
	menu* parent;
};

struct custom_drawn_string_item : custom_drawn_item
{
	custom_drawn_string_item(window* owner, UINT id, menu* parent, const std::wstring& caption_) : custom_drawn_item(owner, id, parent), caption(caption_)
	{
	}

	MENUITEMINFOW get_item_info() const
	{
		MENUITEMINFOW mii(custom_drawn_item::get_item_info());
		mii.fMask |= MIIM_FTYPE | MIIM_STRING;
		mii.fType |= MFT_STRING;
		mii.cch = static_cast<UINT>(caption.size());
		mii.dwTypeData = const_cast<wchar_t*>(caption.c_str());
		return mii;
	}

	std::wstring caption;
};

struct custom_drawn_separator_item : custom_drawn_item
{
	custom_drawn_separator_item(window* owner, UINT id, menu* parent) : custom_drawn_item(owner, id, parent)
	{
	}

	MENUITEMINFOW get_item_info() const
	{
		MENUITEMINFOW mii(custom_drawn_item::get_item_info());
		mii.fMask |= MIIM_FTYPE;
		mii.fType |= MFT_SEPARATOR;
		return mii;
	}
};


struct theme_metrics
{
	theme_metrics(HWND window_) : window(window_), theme(::OpenThemeData(window, VSCLASS_MENU))
	{
		::GetThemePartSize(theme, NULL, MENU_POPUPCHECK, 0, NULL, TS_TRUE, &popup_checkbox_size);
		::GetThemePartSize(theme, NULL, MENU_POPUPSEPARATOR, 0, NULL, TS_TRUE, &popup_separator);

		::GetThemeInt(theme, MENU_POPUPITEM, 0, TMT_BORDERSIZE, &popup_border);
		::GetThemeInt(theme, MENU_POPUPBACKGROUND, 0, TMT_BORDERSIZE, &popup_gutter);

		::GetThemeMargins(theme, NULL, MENU_POPUPCHECK, 0, TMT_CONTENTMARGINS, NULL, &popup_checkbox_margins);
		::GetThemeMargins(theme, NULL, MENU_POPUPCHECKBACKGROUND, 0, TMT_CONTENTMARGINS, NULL, &popup_checkbox_background);
		::GetThemeMargins(theme, NULL, MENU_POPUPITEM, 0, TMT_CONTENTMARGINS, NULL, &popup_item);

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

	void to_measure_size(const SIZE *psizeDraw, const MARGINS *pmargins, __out LPSIZE psizeMeasure)
	{
		psizeMeasure->cx = psizeDraw->cx + pmargins->cxLeftWidth + pmargins->cxRightWidth;
		psizeMeasure->cy = psizeDraw->cy + pmargins->cyTopHeight + pmargins->cyBottomHeight;
	}

	void to_draw_rect(LPCRECT prcMeasure, const MARGINS *pmargins, __out LPRECT prcDraw)
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
		if(metrics.get() == NULL)
		{
			// sigh.  I disable theming on the main window because I want square corners,
			// but this means I no longer know what theme it has loaded.  I hope the desktop
			// window has the right theme, but I don't think it's under any particular obligation to do so.
			metrics.reset(new theme_metrics(::GetDesktopWindow()));
			//metrics.reset(new theme_metrics(get_owning_window()->get_window()));
		}
		return *metrics;
	}

	std::auto_ptr<theme_metrics> metrics;
	SIZE part_sizes[menu_parts_max];
};

namespace
{
	inline int RectWidth(const RECT &r) { return r.right - r.left; }
	inline int RectHeight(const RECT &r) { return r.bottom - r.top; }
}

struct themed_separator_item : themed_item, custom_drawn_separator_item
{
	themed_separator_item(window* owner, UINT id, menu* parent) : custom_drawn_separator_item(owner, id, parent)
	{
	}

	virtual void onMeasureItem(HWND window, MEASUREITEMSTRUCT* measure_item)
	{
		::memset(part_sizes, 0, sizeof(part_sizes));

		SIZE draw_size(get_metrics().popup_checkbox_size);
		draw_size.cy += get_metrics().popup_checkbox_background.cyTopHeight + get_metrics().popup_checkbox_background.cyBottomHeight;
		
		get_metrics().to_measure_size(&draw_size, &get_metrics().popup_checkbox_margins, &part_sizes[menu_checkbox]);
		int cxTotal(part_sizes[menu_checkbox].cx);

		// Size the separator, using the minimum width
		draw_size = get_metrics().popup_checkbox_size;
		draw_size.cy = get_metrics().popup_separator.cy;
		
		get_metrics().to_measure_size(&draw_size, &get_metrics().popup_item, &part_sizes[menu_separator]);

		measure_item->itemWidth = cxTotal;
		measure_item->itemHeight = part_sizes[menu_separator].cy;
	}

	virtual void onDrawItem(HWND window, const DRAWITEMSTRUCT* draw_item)
	{
		MENUITEMINFOW mii = { sizeof(MENUITEMINFOW) };
		mii.fMask = MIIM_CHECKMARKS | MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
		::GetMenuItemInfoW(get_parent()->get_menu(), get_parent()->get_offset(this), TRUE, &mii);

		draw_item_metrics dim = {0};
		const RECT* prcItem(&draw_item->rcItem);
		const int cyItem(RectHeight(draw_item->rcItem));
		const SIZE* prgsize(part_sizes);
		int x(prcItem->left + get_metrics().popup_item.cxLeftWidth);
		const int y(prcItem->top);
		RECT rcMeasure;

		for (UINT i = 0; i < menu_parts_max; i++)
		{
			if(prgsize[i].cx)
			{
				int cx(0);
				int cy(0);

				switch(i)
				{
				case menu_checkbox:
					// Add left padding for the check background rectangle
					x += get_metrics().popup_checkbox_background.cxLeftWidth;

					// Right align the check/bitmap in the column
					cx =  prgsize[i].cx;
					cy =  prgsize[i].cy - get_metrics().popup_checkbox_gutter;
					break;

				default:
					cx = prgsize[i].cx;
					cy = prgsize[i].cy;
					break;
				}

				// Position and vertically center the subitem
				::SetRect(&rcMeasure, x, y, x + cx, y + cy);
				if(i == menu_checkbox)
				{
					get_metrics().to_draw_rect(&rcMeasure, &get_metrics().popup_checkbox_margins, &dim.parts[i]);
				}
				else
				{
					get_metrics().to_draw_rect(&rcMeasure, &get_metrics().popup_text, &dim.parts[i]);
				}
				::OffsetRect(&dim.parts[i], 0, (cyItem - cy) / 2);

				// Add right padding for the check background rectangle
				if(i == menu_checkbox)
				{
					x += get_metrics().popup_checkbox_background.cxRightWidth;
				}

				// Move to the next subitem
				x += cx;
			}
		}

		// Calculate the check background draw size
		SIZE draw_size;
		draw_size.cx = prgsize[menu_checkbox].cx;
		draw_size.cy = prgsize[menu_checkbox].cy - get_metrics().popup_checkbox_gutter;

		// Calculate the check background measure size
		SIZE background_measure;
		get_metrics().to_measure_size(&draw_size, &get_metrics().popup_checkbox_background, &background_measure);

		// Layout the check background rectangle
		x = prcItem->left + get_metrics().popup_item.cxLeftWidth;

		::SetRect(&rcMeasure, x, y, x + background_measure.cx, y + background_measure.cy);
		get_metrics().to_draw_rect(&rcMeasure, &get_metrics().popup_checkbox_background, &dim.check_background);
		::OffsetRect(&dim.check_background, 0, (cyItem - background_measure.cy) / 2);

		// Layout gutter rectangle
		x = prcItem->left;

		draw_size.cx = prgsize[menu_checkbox].cx;
		get_metrics().to_measure_size(&draw_size, &get_metrics().popup_checkbox_background, &background_measure);

		::SetRect(&dim.gutter, x, y, x + get_metrics().popup_item.cxLeftWidth + background_measure.cx, y + cyItem);

		// Layout the separator rectangle
		x = dim.gutter.right + get_metrics().popup_item.cxLeftWidth;

		::SetRect(&rcMeasure, x, y, prcItem->right - get_metrics().popup_item.cxRightWidth, y + prgsize[menu_separator].cy);
		get_metrics().to_draw_rect(&rcMeasure, &get_metrics().popup_item, &dim.parts[menu_separator]);
		::OffsetRect(&dim.parts[menu_separator], 0, (cyItem - prgsize[menu_separator].cy) / 2);

		POPUPITEMSTATES iStateId = get_metrics().to_popup_item_state(draw_item->itemState);
		if(::IsThemeBackgroundPartiallyTransparent(get_metrics().theme, MENU_POPUPITEM, iStateId))
		{
			::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPBACKGROUND, 0, &draw_item->rcItem, NULL);
		}
		::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPGUTTER, 0, &dim.gutter, NULL);
		::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPSEPARATOR, 0, &dim.parts[menu_separator], NULL);
	}
};

struct themed_string_item : themed_item, custom_drawn_string_item
{
	themed_string_item(window* owner, UINT id, menu* parent, const std::wstring& caption) : custom_drawn_string_item(owner, id, parent, caption)
	{
	}

	virtual void onMeasureItem(HWND window, MEASUREITEMSTRUCT* measure_item)
	{
		::memset(part_sizes, 0, sizeof(part_sizes));

		SIZE draw_size(get_metrics().popup_checkbox_size);
		draw_size.cy += get_metrics().popup_checkbox_background.cyTopHeight + get_metrics().popup_checkbox_background.cyBottomHeight;
		
		get_metrics().to_measure_size(&draw_size, &get_metrics().popup_checkbox_margins, &part_sizes[menu_checkbox]);
		int cxTotal(part_sizes[menu_checkbox].cx);

		// Add check background horizontal padding
		cxTotal += get_metrics().popup_checkbox_background.cxLeftWidth + get_metrics().popup_checkbox_background.cxRightWidth;

		HDC dc(::GetDC(get_owning_window()->get_window()));
		ON_BLOCK_EXIT(&::ReleaseDC, get_owning_window()->get_window(), dc);
		// Size the text subitem rectangle
		RECT rcText = { 0 };
		::GetThemeTextExtent(get_metrics().theme, dc, MENU_POPUPITEM, 0, caption.c_str(), caption.size(), DT_LEFT | DT_SINGLELINE | DT_EXPANDTABS, NULL, &rcText);
		draw_size.cx = rcText.right;
		draw_size.cy = rcText.bottom;

		get_metrics().to_measure_size(&draw_size, &get_metrics().popup_text, &part_sizes[menu_text]);
		cxTotal += part_sizes[menu_text].cx;

		// Account for selection margin padding
		cxTotal += get_metrics().popup_item.cxLeftWidth + get_metrics().popup_item.cxRightWidth;
		
		// Calculate maximum menu item height
		LONG cyMax(0);
		for(size_t i(0); i < menu_parts_max; ++i)
		{
			cyMax = std::max(cyMax, part_sizes[i].cy);
		}

		measure_item->itemWidth = cxTotal;
		measure_item->itemHeight = cyMax;
	}

	virtual void onDrawItem(HWND window, const DRAWITEMSTRUCT* draw_item)
	{
		MENUITEMINFOW mii = { sizeof(MENUITEMINFOW) };
		mii.fMask = MIIM_CHECKMARKS | MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
		::GetMenuItemInfoW(get_parent()->get_menu(), get_parent()->get_offset(this), TRUE, &mii);

		draw_item_metrics dim = {0};
		const RECT* prcItem(&draw_item->rcItem);
		const int cyItem(RectHeight(draw_item->rcItem));
		const SIZE* prgsize(part_sizes);
		int x(prcItem->left + get_metrics().popup_item.cxLeftWidth); // Left gutter margin
		const int y(prcItem->top);
		RECT rcMeasure;

		for (UINT i = 0; i < menu_parts_max; i++)
		{
			if(prgsize[i].cx)
			{
				int cx(0);
				int cy(0);

				switch(i)
				{
				case menu_checkbox:
					// Add left padding for the check background rectangle
					x += get_metrics().popup_checkbox_background.cxLeftWidth;

					// Right align the check/bitmap in the column
					cx =  prgsize[i].cx;
					cy =  prgsize[i].cy - get_metrics().popup_checkbox_gutter;
					break;

				default:
					cx = prgsize[i].cx;
					cy = prgsize[i].cy;
					break;
				}

				// Position and vertically center the subitem
				::SetRect(&rcMeasure, x, y, x + cx, y + cy);
				if(i == menu_checkbox)
				{
					get_metrics().to_draw_rect(&rcMeasure, &get_metrics().popup_checkbox_margins, &dim.parts[i]);
				}
				else
				{
					get_metrics().to_draw_rect(&rcMeasure, &get_metrics().popup_text, &dim.parts[i]);
				}
				::OffsetRect(&dim.parts[i], 0, (cyItem - cy) / 2);

				// Add right padding for the check background rectangle
				if(i == menu_checkbox)
				{
					x += get_metrics().popup_checkbox_background.cxRightWidth;
				}

				// Move to the next subitem
				x += cx;
			}
		}

		// Calculate the check background draw size
		SIZE draw_size;
		draw_size.cx = prgsize[menu_checkbox].cx;
		draw_size.cy = prgsize[menu_checkbox].cy - get_metrics().popup_checkbox_gutter;

		// Calculate the check background measure size
		SIZE background_measure;
		get_metrics().to_measure_size(&draw_size, &get_metrics().popup_checkbox_background, &background_measure);

		// Layout the check background rectangle
		x = prcItem->left + get_metrics().popup_item.cxLeftWidth;

		::SetRect(&rcMeasure, x, y, x + background_measure.cx, y + background_measure.cy);
		get_metrics().to_draw_rect(&rcMeasure, &get_metrics().popup_checkbox_background, &dim.check_background);
		::OffsetRect(&dim.check_background, 0, (cyItem - background_measure.cy) / 2);

		// Layout gutter rectangle
		x = prcItem->left;

		draw_size.cx = prgsize[menu_checkbox].cx;
		get_metrics().to_measure_size(&draw_size, &get_metrics().popup_checkbox_background, &background_measure);

		::SetRect(&dim.gutter, x, y, x + get_metrics().popup_item.cxLeftWidth + background_measure.cx, y + cyItem);

		// Layout selection rectangle
		x = prcItem->left + get_metrics().popup_item.cxLeftWidth;

		::SetRect(&dim.selection, x, y, prcItem->right - get_metrics().popup_item.cxRightWidth, y + cyItem);   

		POPUPITEMSTATES iStateId = get_metrics().to_popup_item_state(draw_item->itemState);

		if(::IsThemeBackgroundPartiallyTransparent(get_metrics().theme, MENU_POPUPITEM, iStateId))
		{
			::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPBACKGROUND, 0, &draw_item->rcItem, NULL);
		}

		::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPGUTTER, 0, &dim.gutter, NULL);

		::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPITEM, iStateId, &dim.selection, NULL);
		if(mii.fState & MFS_CHECKED)
		{
			::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPCHECKBACKGROUND, get_metrics().to_popup_check_background_state(iStateId), &dim.check_background, NULL);
			::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPCHECK, get_metrics().to_popup_check_state(mii.fType, iStateId), &dim.parts[menu_checkbox], NULL);
		}
		ULONG accel(((draw_item->itemState & ODS_NOACCEL) ? DT_HIDEPREFIX : 0));

		std::wstring::size_type os(caption.find(L'\t'));
		std::wstring caption_left(caption.substr(0, os));
		::DrawThemeText(get_metrics().theme, draw_item->hDC, MENU_POPUPITEM, iStateId, caption_left.c_str(), caption_left.size(), DT_SINGLELINE | DT_LEFT | accel, 0, &dim.parts[menu_text]);
		if(os != std::wstring::npos)
		{
			std::wstring caption_right(caption.substr(os + 1));
			RECT r = {0};
			int width(::RectWidth(draw_item->rcItem));
			r.left = dim.parts[menu_text].left;
			r.top = dim.parts[menu_text].top;
			r.right = width - ::RectWidth(dim.parts[menu_checkbox]);
			r.bottom = dim.parts[menu_text].bottom;
			::DrawThemeText(get_metrics().theme, draw_item->hDC, MENU_POPUPITEM, iStateId, caption_right.c_str(), caption_right.size(), DT_SINGLELINE | DT_RIGHT | accel, 0, &r);
		}
	}

};

#endif
