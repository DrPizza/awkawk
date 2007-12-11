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

#pragma once

#ifndef PLAYERWINDOW__H
#define PLAYERWINDOW__H

#include "stdafx.h"

#include "window.h"

#include "player_menus.h"

_COM_SMARTPTR_TYPEDEF(IDropTarget, __uuidof(IDropTarget));
_COM_SMARTPTR_TYPEDEF(IDataObject, __uuidof(IDataObject));
_COM_SMARTPTR_TYPEDEF(ISpecifyPropertyPages, __uuidof(ISpecifyPropertyPages));

struct player_window : window, boost::noncopyable
{
	player_window(awkawk* player_);
	~player_window();

	void create_window(int cmd_show);

	std::wstring get_local_path() const;
	std::wstring get_remote_path() const;
	enum
	{
		create_d3d_msg = WM_USER + 1,
		create_device_msg,
		reset_device_msg,
		reset_msg,
		render_msg,
		destroy_device_msg,
		destroy_d3d_msg
	};

	void open_single_file(const std::wstring& path);

	// wndproc and associated message handlers
	LRESULT CALLBACK message_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
	void onCommand(HWND wnd, int id, HWND control, UINT event);
	void onContextMenu(HWND wnd, HWND hwndContext, UINT x, UINT y);
	void onDestroy(HWND wnd);
	void onNCPaint(HWND wnd, HRGN rgn);
	BOOL onNCActivate(HWND wnd, BOOL active, HWND deactivated, BOOL minimized);
	UINT onNCCalcSize(HWND wnd, BOOL calcValidRects, NCCALCSIZE_PARAMS* params);
	UINT onNCHitTest(HWND wnd, int x, int y);
	void onMouseMove(HWND wnd, int x, int y, UINT keyFlags);
	BOOL onSizing(HWND wnd, UINT edge, RECT* coords);
	void onMouseLeave(HWND wnd);
	void onMove(HWND wnd, int x, int y);
	void onSize(HWND wnd, UINT state, int x, int y);
	void onLeftButtonDown(HWND wnd, BOOL doubleClick, int x, int y, UINT keyFlags);
	void onLeftButtonUp(HWND wnd, int x, int y, UINT keyFlags);
	BOOL onMoving(HWND wnd, RECT* coords);
	void onGetMinMaxInfo(HWND wnd, MINMAXINFO* minMaxInfo);
	void onEnterSizeMove(HWND wnd);
	void onExitSizeMove(HWND wnd);
	BOOL onEraseBackground(HWND wnd, HDC hdc);
	void onPaint(HWND wnd);
	void onInitMenu(HWND wnd, HMENU menu);
	void onInitMenuPopup(HWND wnd, HMENU menu, UINT item, BOOL window_menu);
	void onSysCommand(HWND wnd, UINT command, int x, int y);
	void onTimer(HWND wnd, UINT id);

	void hide_cursor() const
	{
		while(::ShowCursor(FALSE) >= 0)
		{
		}
	}

	void show_cursor() const
	{
		while(::ShowCursor(TRUE) < 0)
		{
		}
	}

	void resize_window(int newWidth, int newHeight);

	struct drop_target : IDropTarget
	{
		drop_target(player_window* player_window_) : ref_count(0), player_window(player_window_), drag_effect(DROPEFFECT_NONE)
		{
		}
		// IUnknown
		STDMETHODIMP QueryInterface(const IID& iid, void** ppvObject);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();
		// IDropTarget
		virtual STDMETHODIMP DragEnter(IDataObject* dataObject, DWORD keyState, POINTL pt, DWORD* allowed);
		virtual STDMETHODIMP DragOver(DWORD keyState, POINTL pt, DWORD* effect);
		virtual STDMETHODIMP DragLeave();
		virtual STDMETHODIMP Drop(IDataObject* dataObject, DWORD keyState, POINTL pt, DWORD* effect);
		virtual ~drop_target()
		{
		}
	private:
		bool is_droppable(IDataObjectPtr dataObject);
		_bstr_t get_file_name(IDataObjectPtr dataObject);
		long ref_count;
		player_window* player_window;
		DWORD drag_effect;
	};

	const std::wstring& get_app_title() const
	{
		return app_title;
	}

private:
	awkawk* player;

	// window, message loop and associated paraphernalia
	std::wstring app_title;
	awkawk_main_menu main_menu;
	HMENU filter_menu;

	// mouse position tracking
	bool tracking;

	// dragging + snapping
	bool dragging; // if we're in a drag
	POINT old_position; // position at drag start
	POINT snap_offset;

	enum { mouse_kill = 1 };
	LARGE_INTEGER last_mouse_move_time;

	void update_last_mouse_move_time();

	IDropTargetPtr target;
};

#endif
