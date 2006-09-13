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

#ifndef WINDOW__H
#define WINDOW__H

#define NOMINMAX
#define _WIN32_WINNT 0x0502
#define WINVER 0x0502
#define STRICT
#pragma warning(disable:4995)
#pragma warning(disable:4996)

#include <objbase.h>
#include <windows.h>
#include <windowsx.h>

#include <set>

#include "util.h"

struct window;

struct message_handler
{
	message_handler(window* parent_) : parent(parent_)
	{
	}

	virtual bool handles_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		return false;
	}
	virtual LRESULT CALLBACK message_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;

	window* parent;
};

struct window : message_handler
{
	window() : message_handler(NULL), instance(::GetModuleHandle(NULL)), accelerator_table(NULL)
	{
	}

	window(const wchar_t* accelerators_id) : message_handler(NULL), instance(::GetModuleHandle(NULL)), accelerator_table(::LoadAccelerators(instance, accelerators_id))
	{
	}

	virtual ~window()
	{
		if(accelerator_table != NULL)
		{
			::DestroyAcceleratorTable(accelerator_table);
		}
	}

	void register_class(UINT style, HICON icon, HCURSOR cursor, HBRUSH background, const wchar_t* menu_name, const wchar_t* class_name)
	{
		WNDCLASSEX wc = {0};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = style;
		wc.lpfnWndProc = &window::message_proc_helper;
		wc.cbWndExtra = sizeof(window*);
		wc.hInstance = instance;
		wc.hIcon = icon;
		wc.hCursor = cursor;
		wc.hbrBackground = background;
		wc.lpszMenuName = menu_name;
		wc.lpszClassName = class_name;
		window_class = ::RegisterClassEx(&wc);
		if(0 == window_class && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
		{
			throw std::runtime_error("Could not register window class");
		}
	}

	static LRESULT CALLBACK message_proc_helper(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)
		{
		case WM_CREATE:
			{
				CREATESTRUCT* cs(reinterpret_cast<CREATESTRUCT*>(lParam));
				::SetWindowLongPtr(wnd, 0, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
			}
		default:
			{
				window* w(reinterpret_cast<window*>(::GetWindowLongPtr(wnd, 0)));
				if(w)
				{
					return w->filtering_message_proc(wnd, message, wParam, lParam);
				}
				else
				{
					return ::DefWindowProc(wnd, message, wParam, lParam);
				}
			}
		}
	}

	int APIENTRY pump_messages()
	{
		MSG msg = {0};
		while(::GetMessage(&msg, NULL, 0, 0))
		{
			if(!::TranslateAccelerator(msg.hwnd, accelerator_table, &msg))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
		return static_cast<int>(msg.wParam);
	}

	LRESULT CALLBACK filtering_message_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		for(std::set<message_handler*>::iterator it(message_handlers.begin()), end(message_handlers.end()); it != end; ++it)
		{
			if((*it)->handles_message(wnd, message, wParam, lParam))
			{
				return (*it)->message_proc(wnd, message, wParam, lParam);
			}
		}
		return message_proc(wnd, message, wParam, lParam);
	}

	HWND get_window() const
	{
		return window_handle;
	}

	void set_window(HWND wnd)
	{
		window_handle = wnd;
	}

	void destroy_window()
	{
		::DestroyWindow(get_window());
	}

	void invalidate() const
	{
		::InvalidateRect(get_window(), NULL, TRUE);
	}

	void resize_window(int newWidth, int newHeight)
	{
		::SetWindowPos(get_window(), HWND_TOP, 0, 0, newWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOSENDCHANGING);
	}

	bool is_minimized() const
	{
		return ::IsIconic(get_window()) == TRUE;
	}

	void set_on_top(bool ontop) const
	{
		if(ontop)
		{
			::SetWindowPos(get_window(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
		}
		else
		{
			::SetWindowPos(get_window(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
		}
	}

	void move_window(int x, int y)
	{
		::SetWindowPos(get_window(), HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOSENDCHANGING);
	}

	void offset_window(int delta_x, int delta_y)
	{
		RECT window_position(get_window_rect());
		::SetWindowPos(get_window(), HWND_TOP, window_position.left + delta_x, window_position.top + delta_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOSENDCHANGING);
	}

	void add_message_handler(message_handler* new_handler)
	{
		message_handlers.insert(new_handler);
	}

	void remove_message_handler(message_handler* old_handler)
	{
		message_handlers.erase(old_handler);
	}

	RECT get_window_rect() const
	{
		RECT wr = {0};
		::GetWindowRect(get_window(), &wr);
		return wr;
	}

	BOOL post_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ::PostMessage(get_window(), msg, wParam, lParam);
	}

	LRESULT send_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ::SendMessage(get_window(), msg, wParam, lParam);
	}

	BOOL show_window(int cmd_show)
	{
		return ::ShowWindow(get_window(), cmd_show);
	}

	BOOL set_placement(WINDOWPLACEMENT* placement)
	{
		placement->length = sizeof(WINDOWPLACEMENT);
		return ::SetWindowPlacement(get_window(), placement);
	}

	BOOL get_placement(WINDOWPLACEMENT* placement)
	{
		placement->length = sizeof(WINDOWPLACEMENT);
		return ::GetWindowPlacement(get_window(), placement);
	}

	HINSTANCE instance;

private:
	HWND window_handle;
	std::set<message_handler*> message_handlers;
	HACCEL accelerator_table;
	ATOM window_class;
};


struct dialogue : message_handler
{
	dialogue() : message_handler(NULL), instance(::GetModuleHandle(NULL))
	{
	}

	virtual ~dialogue()
	{
	}

	void register_class(UINT style, HICON icon, HCURSOR cursor, HBRUSH background, const wchar_t* menu_name, const wchar_t* class_name)
	{
		WNDCLASSEX wc = {0};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = style;
		wc.lpfnWndProc = &dialogue::message_proc_helper;
		wc.cbWndExtra = DLGWINDOWEXTRA + sizeof(dialogue*);
		wc.hInstance = instance;
		wc.hIcon = icon;
		wc.hCursor = cursor;
		wc.hbrBackground = background;
		wc.lpszMenuName = menu_name;
		wc.lpszClassName = class_name;
		window_class = ::RegisterClassEx(&wc);
		if(0 == window_class && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
		{
			throw std::runtime_error("Could not register window class");
		}
	}

	INT_PTR display_dialogue(const wchar_t* template_name, HWND parent)
	{
		return ::DialogBoxParam(instance, template_name, parent, &dialogue::dummy_dialogue_proc, reinterpret_cast<LPARAM>(this));
	}

	static INT_PTR CALLBACK dummy_dialogue_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		return FALSE;
	}

	static LRESULT CALLBACK message_proc_helper(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)
		{
		case WM_INITDIALOG:
			{
				dialogue* d(reinterpret_cast<dialogue*>(lParam));
				d->set_window(wnd);
				::SetWindowLongPtr(wnd, DLGWINDOWEXTRA, reinterpret_cast<LONG_PTR>(d));
			}
		default:
			{
				dialogue* d(reinterpret_cast<dialogue*>(::GetWindowLongPtr(wnd, DLGWINDOWEXTRA)));
				if(d)
				{
					return d->filtering_message_proc(wnd, message, wParam, lParam);
				}
				else
				{
					return ::DefDlgProc(wnd, message, wParam, lParam);
				}
			}
		}
	}

	LRESULT CALLBACK filtering_message_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		for(std::set<message_handler*>::iterator it(message_handlers.begin()), end(message_handlers.end()); it != end; ++it)
		{
			if((*it)->handles_message(wnd, message, wParam, lParam))
			{
				return (*it)->message_proc(wnd, message, wParam, lParam);
			}
		}
		return message_proc(wnd, message, wParam, lParam);
	}

	HWND get_window() const
	{
		return window_handle;
	}

	void set_window(HWND wnd)
	{
		window_handle = wnd;
	}

	void destroy_window()
	{
		::DestroyWindow(get_window());
	}

	void invalidate() const
	{
		::InvalidateRect(get_window(), NULL, TRUE);
	}

	void resize_window(int newWidth, int newHeight)
	{
		::SetWindowPos(get_window(), HWND_TOP, 0, 0, newWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOSENDCHANGING);
	}

	bool is_minimized() const
	{
		return ::IsIconic(get_window()) == TRUE;
	}

	void set_on_top(bool ontop) const
	{
		if(ontop)
		{
			::SetWindowPos(get_window(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
		}
		else
		{
			::SetWindowPos(get_window(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
		}
	}

	void move_window(int x, int y)
	{
		::SetWindowPos(get_window(), HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOSENDCHANGING);
	}

	void offset_window(int delta_x, int delta_y)
	{
		RECT window_position(get_window_rect());
		::SetWindowPos(get_window(), HWND_TOP, window_position.left + delta_x, window_position.top + delta_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOSENDCHANGING);
	}

	void add_message_handler(message_handler* new_handler)
	{
		message_handlers.insert(new_handler);
	}

	void remove_message_handler(message_handler* old_handler)
	{
		message_handlers.erase(old_handler);
	}

	RECT get_window_rect() const
	{
		RECT wr = {0};
		::GetWindowRect(get_window(), &wr);
		return wr;
	}

	BOOL post_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ::PostMessage(get_window(), msg, wParam, lParam);
	}

	LRESULT send_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ::SendMessage(get_window(), msg, wParam, lParam);
	}

	BOOL show_window(int cmd_show)
	{
		return ::ShowWindow(get_window(), cmd_show);
	}

	BOOL set_placement(WINDOWPLACEMENT* placement)
	{
		placement->length = sizeof(WINDOWPLACEMENT);
		return ::SetWindowPlacement(get_window(), placement);
	}

	BOOL get_placement(WINDOWPLACEMENT* placement)
	{
		placement->length = sizeof(WINDOWPLACEMENT);
		return ::GetWindowPlacement(get_window(), placement);
	}

	HINSTANCE instance;

private:
	HWND window_handle;
	std::set<message_handler*> message_handlers;
	ATOM window_class;
};

#endif
