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

#include "menu.h"

namespace
{
	inline int RectWidth(const RECT &r) { return r.right - r.left; }
	inline int RectHeight(const RECT &r) { return r.bottom - r.top; }
}

void themed_separator_item::onMeasureItem(HWND, MEASUREITEMSTRUCT* measure_item)
{
	if(::IsAppThemed() != TRUE)
	{
		int width(::GetSystemMetrics(SM_CXMENUCHECK));
		int height(::GetSystemMetrics(SM_CYMENUCHECK));
		int edge_width(::GetSystemMetrics(SM_CXEDGE));
		int edge_height(::GetSystemMetrics(SM_CYEDGE));

		measure_item->itemWidth = (width * 2) + (edge_width * 2);
		measure_item->itemHeight = height + (edge_height * 2);
		return;
	}
	::memset(part_sizes, 0, sizeof(part_sizes));

	SIZE draw_size(get_metrics().popup_checkbox_size);
	draw_size.cy += get_metrics().popup_checkbox_background.cyTopHeight + get_metrics().popup_checkbox_background.cyBottomHeight;
	
	get_metrics().to_measure_size(&draw_size, &get_metrics().popup_checkbox_margins, &part_sizes[menu_checkbox]);
	int cxTotal(part_sizes[menu_checkbox].cx);

	// Size the separator, using the minimum width
	draw_size = get_metrics().popup_checkbox_size;
	draw_size.cy = get_metrics().popup_separator.cy;
	
	get_metrics().to_measure_size(&draw_size, &get_metrics().popup_item, &part_sizes[themed_item::menu_separator]);

	measure_item->itemWidth = cxTotal;
	measure_item->itemHeight = part_sizes[themed_item::menu_separator].cy;
}

void themed_separator_item::onDrawItem(HWND window, const DRAWITEMSTRUCT* draw_item)
{
	if(::IsAppThemed() != TRUE)
	{
		// TODO fixme
		return FORWARD_WM_DRAWITEM(window, draw_item, ::DefWindowProcW);
	}
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

	for(UINT i(0); i < menu_parts_max; i++)
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

	::SetRect(&rcMeasure, x, y, prcItem->right - get_metrics().popup_item.cxRightWidth, y + prgsize[themed_item::menu_separator].cy);
	get_metrics().to_draw_rect(&rcMeasure, &get_metrics().popup_item, &dim.parts[themed_item::menu_separator]);
	::OffsetRect(&dim.parts[themed_item::menu_separator], 0, (cyItem - prgsize[themed_item::menu_separator].cy) / 2);

	POPUPITEMSTATES iStateId = get_metrics().to_popup_item_state(draw_item->itemState);
	if(::IsThemeBackgroundPartiallyTransparent(get_metrics().theme, MENU_POPUPITEM, iStateId))
	{
		::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPBACKGROUND, 0, &draw_item->rcItem, nullptr);
	}
	::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPGUTTER, 0, &dim.gutter, nullptr);
	::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPSEPARATOR, 0, &dim.parts[themed_item::menu_separator], nullptr);
}

void themed_string_item::onMeasureItem(HWND, MEASUREITEMSTRUCT* measure_item)
{
	if(::IsAppThemed() != TRUE)
	{

		int width(::GetSystemMetrics(SM_CXMENUCHECK));
		int height(::GetSystemMetrics(SM_CYMENUCHECK));
		int edge_width(::GetSystemMetrics(SM_CXEDGE));
		int edge_height(::GetSystemMetrics(SM_CYEDGE));

		measure_item->itemWidth = (width * 2) + (edge_width * 4);
		measure_item->itemHeight = height + (edge_height * 2);

		NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
		::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(SPI_GETNONCLIENTMETRICS), &ncm, 0);
		HFONT menu_font(::CreateFontIndirectW(&ncm.lfMenuFont));
		ON_BLOCK_EXIT(::DeleteObject(menu_font));

		HDC dc(::GetDC(get_owning_window()->get_window()));
		ON_BLOCK_EXIT(::ReleaseDC(get_owning_window()->get_window(), dc));
		HGDIOBJ previous_font(::SelectObject(dc, menu_font));
		ON_BLOCK_EXIT(::SelectObject(dc, previous_font));
		
		SIZE text_size = {0};
		::GetTextExtentPointW(dc, get_caption().c_str(), static_cast<int>(get_caption().size()), &text_size);
		measure_item->itemWidth += text_size.cx;
		measure_item->itemHeight = ncm.iMenuHeight;
		return;
	}
	::memset(part_sizes, 0, sizeof(part_sizes));

	SIZE draw_size(get_metrics().popup_checkbox_size);
	draw_size.cy += get_metrics().popup_checkbox_background.cyTopHeight + get_metrics().popup_checkbox_background.cyBottomHeight;
	
	get_metrics().to_measure_size(&draw_size, &get_metrics().popup_checkbox_margins, &part_sizes[menu_checkbox]);
	int cxTotal(part_sizes[menu_checkbox].cx);

	// Add check background horizontal padding
	cxTotal += get_metrics().popup_checkbox_background.cxLeftWidth + get_metrics().popup_checkbox_background.cxRightWidth;

	HDC dc(::GetDC(get_owning_window()->get_window()));
	ON_BLOCK_EXIT(::ReleaseDC(get_owning_window()->get_window(), dc));
	// Size the text subitem rectangle
	RECT rcText = { 0 };
	::GetThemeTextExtent(get_metrics().theme, dc, MENU_POPUPITEM, 0, get_caption().c_str(), static_cast<int>(get_caption().size()), DT_LEFT | DT_SINGLELINE | DT_EXPANDTABS, nullptr, &rcText);
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

void themed_string_item::onDrawItem(HWND, const DRAWITEMSTRUCT* draw_item)
{
	MENUITEMINFOW mii = { sizeof(MENUITEMINFOW) };
	mii.fMask = MIIM_CHECKMARKS | MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
	::GetMenuItemInfoW(get_parent()->get_menu(), get_parent()->get_offset(this), TRUE, &mii);

	if(::IsAppThemed() != TRUE)
	{
		int width(::GetSystemMetrics(SM_CXMENUCHECK));
		int height(::GetSystemMetrics(SM_CYMENUCHECK));
		int edge_width(::GetSystemMetrics(SM_CXEDGE));
		int edge_height(::GetSystemMetrics(SM_CYEDGE));

		NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
		::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(SPI_GETNONCLIENTMETRICS), &ncm, 0);
		HFONT menu_font(::CreateFontIndirectW(&ncm.lfMenuFont));
		ON_BLOCK_EXIT(::DeleteObject(menu_font));

		HGDIOBJ previous_font(::SelectObject(draw_item->hDC, menu_font));
		ON_BLOCK_EXIT(::SelectObject(draw_item->hDC, previous_font));

		RECT r(draw_item->rcItem);
		::InflateRect(&r, -edge_width, -edge_height);
		::InflateRect(&r, -width, 0);
		::InflateRect(&r, -edge_width, 0);

		const bool selected(draw_item->itemState & ODS_SELECTED);
		const COLORREF foreground_colour(selected ? ::GetSysColor(COLOR_HIGHLIGHTTEXT)
		                                          : ::GetSysColor(COLOR_MENUTEXT));
		HBRUSH foreground_brush(selected ? ::GetSysColorBrush(COLOR_HIGHLIGHTTEXT)
		                                 : ::GetSysColorBrush(COLOR_MENUTEXT));
		const COLORREF background_colour(selected ? ::GetSysColor(COLOR_HIGHLIGHT)
		                                          : ::GetSysColor(COLOR_MENU));
		HBRUSH background_brush(selected ? ::GetSysColorBrush(COLOR_HIGHLIGHT)
		                                 : ::GetSysColorBrush(COLOR_MENU));

		COLORREF previous_colour(::SetTextColor(draw_item->hDC, foreground_colour));
		ON_BLOCK_EXIT(::SetTextColor(draw_item->hDC, previous_colour));
		COLORREF previous_background(::SetBkColor(draw_item->hDC, background_colour));
		ON_BLOCK_EXIT(::SetBkColor(draw_item->hDC, previous_background));

		::FillRect(draw_item->hDC, &draw_item->rcItem, background_brush);

		if(mii.fState & MFS_CHECKED)
		{
			HDC mem(::CreateCompatibleDC(draw_item->hDC));
			ON_BLOCK_EXIT(::DeleteDC(mem));
			HBITMAP checkbox(::CreateBitmap(width, height, 1, 1, nullptr));
			HGDIOBJ previous_bitmap(::SelectObject(mem, checkbox));
			ON_BLOCK_EXIT(::SelectObject(mem, previous_bitmap));
			RECT s = { 0, 0, width, height };
			::DrawFrameControl(mem, &s, DFC_MENU, (mii.fType & MFT_RADIOCHECK) ? DFCS_MENUBULLET : DFCS_MENUCHECK);
			::BitBlt(draw_item->hDC, edge_width, edge_height + r.top + ((::RectHeight(r) - height) / 2), width, height, mem, 0, 0, SRCCOPY);
		}

		std::wstring::size_type os(get_caption().find(L'\t'));
		std::wstring caption_left(get_caption().substr(0, os));
		std::wstring caption_right(get_caption().substr(os + 1));
		if(!(mii.fState & MFS_DISABLED))
		{
			const ULONG accel((draw_item->itemState & ODS_NOACCEL) ? DT_HIDEPREFIX : 0);
			::DrawTextW(draw_item->hDC, caption_left.c_str(), static_cast<int>(caption_left.size()), &r, DT_SINGLELINE | DT_LEFT | DT_EXPANDTABS | accel);
			if(os != std::wstring::npos)
			{
				::DrawTextW(draw_item->hDC, caption_right.c_str(), static_cast<int>(caption_right.size()), &r, DT_SINGLELINE | DT_RIGHT | DT_EXPANDTABS | accel);
			}
		}
		else
		{
			const ULONG accel((draw_item->itemState & ODS_NOACCEL) ? DSS_HIDEPREFIX : 0);
			const ULONG action(selected ? DSS_MONO : DSS_DISABLED);
			::DrawStateW(draw_item->hDC, ::GetSysColorBrush(COLOR_GRAYTEXT), nullptr, reinterpret_cast<LPARAM>(caption_left.c_str()), static_cast<WPARAM>(caption_left.size()), r.left, r.top, 0, 0, DST_PREFIXTEXT | action | accel);
			if(os != std::wstring::npos)
			{
				::DrawStateW(draw_item->hDC, ::GetSysColorBrush(COLOR_GRAYTEXT), nullptr, reinterpret_cast<LPARAM>(caption_right.c_str()), static_cast<WPARAM>(caption_right.size()), r.left, r.top, 0, 0, DST_PREFIXTEXT | DSS_RIGHT | action | accel);
			}
		}
		return;
	}

	draw_item_metrics dim = {0};
	const RECT* prcItem(&draw_item->rcItem);
	const int cyItem(RectHeight(draw_item->rcItem));
	const SIZE* prgsize(part_sizes);
	int x(prcItem->left + get_metrics().popup_item.cxLeftWidth); // Left gutter margin
	const int y(prcItem->top);
	RECT rcMeasure;

	for(UINT i = 0; i < menu_parts_max; i++)
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
		::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPBACKGROUND, 0, &draw_item->rcItem, nullptr);
	}

	::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPGUTTER, 0, &dim.gutter, nullptr);

	::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPITEM, iStateId, &dim.selection, nullptr);
	if(mii.fState & MFS_CHECKED)
	{
		::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPCHECKBACKGROUND, get_metrics().to_popup_check_background_state(iStateId), &dim.check_background, nullptr);
		::DrawThemeBackground(get_metrics().theme, draw_item->hDC, MENU_POPUPCHECK, get_metrics().to_popup_check_state(mii.fType, iStateId), &dim.parts[menu_checkbox], nullptr);
	}
	ULONG accel(((draw_item->itemState & ODS_NOACCEL) ? DT_HIDEPREFIX : 0));

	std::wstring::size_type os(get_caption().find(L'\t'));
	std::wstring caption_left(get_caption().substr(0, os));
	::DrawThemeText(get_metrics().theme, draw_item->hDC, MENU_POPUPITEM, iStateId, caption_left.c_str(), static_cast<int>(caption_left.size()), DT_SINGLELINE | DT_LEFT | accel, 0, &dim.parts[menu_text]);
	if(os != std::wstring::npos)
	{
		std::wstring caption_right(get_caption().substr(os + 1));
		RECT r = {0};
		int width(::RectWidth(draw_item->rcItem));
		r.left = dim.parts[menu_text].left;
		r.top = dim.parts[menu_text].top;
		r.right = width - ::RectWidth(dim.parts[menu_checkbox]);
		r.bottom = dim.parts[menu_text].bottom;
		::DrawThemeText(get_metrics().theme, draw_item->hDC, MENU_POPUPITEM, iStateId, caption_right.c_str(), static_cast<int>(caption_right.size()), DT_SINGLELINE | DT_RIGHT | accel, 0, &r);
	}
}
