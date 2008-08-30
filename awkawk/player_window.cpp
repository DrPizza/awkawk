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

#include "stdafx.h"

#include "player_window.h"
#include "open_url.h"
#include "awkawk.h"
#include "player_direct_show.h"
#include "resource.h"

player_window::player_window(awkawk* player_) : window(L"awkawk class", CS_DBLCLKS, ::LoadIconW(instance, MAKEINTRESOURCEW(IDI_PLAYER)), ::LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW)), static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)), NULL, MAKEINTRESOURCEW(IDR_ACCELERATORS)),
                                                player(player_),
                                                tracking(false),
                                                dragging(false),
                                                main_menu(player, this)
{
	boost::scoped_array<wchar_t> buffer(new wchar_t[256]);
	::LoadStringW(instance, IDS_APP_TITLE, buffer.get(), 256);
	app_title = buffer.get();
}

player_window::~player_window()
{
}

std::wstring player_window::get_local_path() const
{
	OPENFILENAMEW ofn = {0};
	boost::scoped_array<wchar_t> buffer(new wchar_t[0xffff]);
	buffer[0] = L'\0';

	static const wchar_t filter[] = L"Video Files (.asf, .avi, .mpg, .mpeg, .vob, .qt, .wmv, .mp4)\0*.ASF;*.AVI;*.MPG;*.MPEG;*.VOB;*.QT;*.WMV;*.MP4\0"
	                                L"All Files (*.*)\0*.*\0";
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = get_window();
	ofn.hInstance = NULL;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.lpstrFile = buffer.get();
	ofn.nMaxFile = 0xffff;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = L"Select a video file to play...";
	ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = L"AVI";
	ofn.lCustData = 0L;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
	
	if(::GetOpenFileNameW(&ofn) == TRUE)
	{
		return buffer.get();
	}
	return L"";
}

std::wstring player_window::get_remote_path() const
{
	open_url_dialogue dlg;
	switch(dlg.display_dialogue(MAKEINTRESOURCEW(IDD_OPEN_URL), get_window()))
	{
	case open_url_dialogue::cancelled:
		return L"";
	case open_url_dialogue::url:
		return dlg.get_location().c_str();
	case open_url_dialogue::browse_local:
		return get_local_path();
	default:
		__assume(0);
	}
}

void player_window::open_single_file(const std::wstring& path)
{
	if(path.size() > 0)
	{
		boost::scoped_array<wchar_t> buffer(new wchar_t[path.size() + 1]);
		std::memset(buffer.get(), 0, sizeof(wchar_t) * (path.size() + 1));
		std::copy(path.begin(), path.end(), buffer.get());
		::PathStripPathW(buffer.get());
		set_window_text(buffer.get());
		player->open_single_file(path);
	}
}

void player_window::create_window(int cmd_show)
{
	// nb: if I enable WS_EX_COMPOSITED, Vista is unable to update the window when DWM is *dis*abled.  Silent failures.  How amusing.
	// With DWM enabled it works just fine and dandy.  I suspect the "correct" behaviour is to render offscreen and blt to my window.  Poop on that.
	window::create_window(0, app_title.c_str(), WS_SYSMENU | WS_THICKFRAME, 100, 100, player->get_window_dimensions().cx, player->get_window_dimensions().cy, NULL, NULL, NULL);
	if(!get_window())
	{
		throw std::runtime_error("Could not create window");
	}
	set_window_theme(L"", L"");
	HMODULE dwmapi_dll(::LoadLibraryW(L"dwmapi.dll"));
	if(dwmapi_dll != NULL)
	{
		ON_BLOCK_EXIT(&::FreeLibrary, dwmapi_dll);
		typedef HRESULT (STDAPICALLTYPE *DWMSETWINDOWATTRIBUTE)(HWND, DWORD, const void*, DWORD);
		DWMSETWINDOWATTRIBUTE dwm_set_window_attribute(reinterpret_cast<DWMSETWINDOWATTRIBUTE>(::GetProcAddress(dwmapi_dll, "DwmSetWindowAttribute")));
		DWMNCRENDERINGPOLICY policy(DWMNCRP_DISABLED);
		dwm_set_window_attribute(get_window(), DWMWA_NCRENDERING_POLICY, &policy, sizeof(DWMNCRENDERINGPOLICY));
	}

	player->set_window_dimensions(get_window_size());

	drop_target* t(new drop_target(this));
	target.Attach(t, true);
	::RegisterDragDrop(get_window(), target);

	show_window(cmd_show);
	update_window();
	// for reasons I don't understand, the window appears black (functional, just non-drawing) when spawned from the command-line
	// unless I send a WM_NCCALCSIZE (I think).  This does the job, but I have no idea why.  When spawned from the GUI there's no
	// problem either way.
	::SetWindowPos(get_window(), 0, 0, 0, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

LRESULT CALLBACK player_window::message_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	switch(message)
	{
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDBLCLK:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		update_last_mouse_move_time();
		break;
	case WM_TIMER:
		handled = false;
		break;
	}
	switch(message)
	{
	HANDLE_MSG(window, WM_COMMAND, onCommand);
	HANDLE_MSG(window, WM_CONTEXTMENU, onContextMenu);
	HANDLE_MSG(window, WM_DESTROY, onDestroy);
	HANDLE_MSG(window, WM_NCPAINT, onNCPaint);
	HANDLE_MSG(window, WM_NCCALCSIZE, onNCCalcSize);
	HANDLE_MSG(window, WM_NCACTIVATE, onNCActivate);
	HANDLE_MSG(window, WM_NCHITTEST, onNCHitTest);
	HANDLE_MSG(window, WM_MOUSEMOVE, onMouseMove);
	HANDLE_MSG(window, WM_MOUSELEAVE, onMouseLeave);
	HANDLE_MSG(window, WM_SIZE, onSize);
	HANDLE_MSG(window, WM_SIZING, onSizing);
	HANDLE_MSG(window, WM_MOVE, onMove);
	HANDLE_MSG(window, WM_MOVING, onMoving);
	HANDLE_MSG(window, WM_ENTERSIZEMOVE, onEnterSizeMove);
	HANDLE_MSG(window, WM_EXITSIZEMOVE, onExitSizeMove);
	HANDLE_MSG(window, WM_LBUTTONDOWN, onLeftButtonDown);
	HANDLE_MSG(window, WM_LBUTTONDBLCLK, onLeftButtonDown);
	HANDLE_MSG(window, WM_LBUTTONUP, onLeftButtonUp);
	HANDLE_MSG(window, WM_GETMINMAXINFO, onGetMinMaxInfo);
	HANDLE_MSG(window, WM_ERASEBKGND, onEraseBackground);
	HANDLE_MSG(window, WM_PAINT, onPaint);
	HANDLE_MSG(window, WM_INITMENU, onInitMenu);
	HANDLE_MSG(window, WM_INITMENUPOPUP, onInitMenuPopup);
	HANDLE_MSG(window, WM_SYSCOMMAND, onSysCommand);
	HANDLE_MSG(window, WM_TIMER, onTimer);

	case create_d3d_msg:
	case create_device_msg:
	case reset_device_msg:
	case render_msg:
	case reset_msg:
	case destroy_device_msg:
	case destroy_d3d_msg:
		try
		{
			switch(message)
			{
			case create_d3d_msg:
				player->create_d3d();
				break;
			case create_device_msg:
				player->create_device();
				break;
			case reset_device_msg:
				player->reset_device();
				break;
			case reset_msg:
				player->reset();
				break;
			case render_msg:
				player->render();
				break;
			case destroy_device_msg:
				player->destroy_device();
				break;
			case destroy_d3d_msg:
				player->destroy_d3d();
				break;
			}
		}
		catch(_com_error& ce)
		{
			derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
		}
	default:
		handled = false;
	}
	return 0;
}

void player_window::onTimer(HWND, UINT id)
{
	switch(id)
	{
	case mouse_kill:
		{
			static LARGE_INTEGER freq = {0};
			if(freq.QuadPart == 0)
			{
				::QueryPerformanceFrequency(&freq);
			}

			if(get_window() == ::GetForegroundWindow())
			{
				LARGE_INTEGER now = {0};
				::QueryPerformanceCounter(&now);
				if(((now.QuadPart - last_mouse_move_time.QuadPart) / freq.QuadPart) > 3)
				{
					hide_cursor();
				}
			}
			else
			{
				update_last_mouse_move_time();
			}
		}
		break;
	default:
		FORWARD_WM_TIMER(get_window(), id, &::DefWindowProcW);
		break;
	}
}

void player_window::onInitMenu(HWND, HMENU menu)
{
}

void player_window::onInitMenuPopup(HWND, HMENU menu, UINT item, BOOL window_menu)
{
	if(TRUE == window_menu)
	{
		HMENU system_menu(::GetSystemMenu(get_window(), FALSE));
		if(player->is_fullscreen())
		{
			::EnableMenuItem(system_menu, SC_RESTORE, MF_ENABLED);
			::EnableMenuItem(system_menu, SC_MOVE, MF_GRAYED);
			::EnableMenuItem(system_menu, SC_SIZE, MF_GRAYED);
			::EnableMenuItem(system_menu, SC_MINIMIZE, MF_ENABLED);
			::EnableMenuItem(system_menu, SC_MAXIMIZE, MF_GRAYED);
			::EnableMenuItem(system_menu, SC_CLOSE, MF_ENABLED);
		}
		else
		{
			::EnableMenuItem(system_menu, SC_RESTORE, MF_GRAYED);
			::EnableMenuItem(system_menu, SC_MOVE, MF_ENABLED);
			::EnableMenuItem(system_menu, SC_SIZE, MF_ENABLED);
			::EnableMenuItem(system_menu, SC_MINIMIZE, MF_ENABLED);
			::EnableMenuItem(system_menu, SC_MAXIMIZE, MF_ENABLED);
			::EnableMenuItem(system_menu, SC_CLOSE, MF_ENABLED);
		}
	}
}

void player_window::onSysCommand(HWND, UINT command, int x, int y)
{
	switch(command)
	{
	case SC_MAXIMIZE:
		player->set_fullscreen(true);
		break;
	case SC_RESTORE:
		player->set_fullscreen(false);
		break;
	case SC_SCREENSAVE:
		if(player->is_fullscreen())
		{
			return;
		}
		break;
	}
	FORWARD_WM_SYSCOMMAND(get_window(), command, x, y, &::DefWindowProcW);
}

BOOL player_window::onEraseBackground(HWND, HDC)
{
	return TRUE;
}

void player_window::onCommand(HWND, int id, HWND control, UINT event)
{
	try
	{
		switch(id)
		{
			// main menu
		case IDM_OPEN_FILE:
		case IDM_OPEN_URL:
			{
				open_single_file(id == IDM_OPEN_FILE ? get_local_path() : get_remote_path());
			}
			break;
		case IDM_CLOSE_FILE:
			{
				set_window_text(app_title.c_str());
				player->send_message(awkawk::stop);
				player->send_message(awkawk::unload);
				player->clear_files();
			}
			break;
		case IDM_EXIT:
			destroy_window();
			break;
			// play menu
		case IDM_PLAY:
			player->post_message(awkawk::play);
			break;
		case IDM_PAUSE:
			player->post_message(awkawk::pause);
			break;
		case IDM_STOP:
			player->post_message(awkawk::stop);
			break;
			// mode menu
		case IDM_PLAYMODE_NORMAL:
			player->set_playmode(player_playlist::normal);
			break;
		case IDM_PLAYMODE_REPEATALL:
			player->set_playmode(player_playlist::repeat_all);
			break;
		case IDM_PLAYMODE_REPEATTRACK:
			player->set_playmode(player_playlist::repeat_single);
			break;
		case IDM_PLAYMODE_SHUFFLE:
			player->set_playmode(player_playlist::shuffle);
			break;
			// size menu
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
			// miscellaneous
		case IDM_NEXT:
			player->post_message(awkawk::next);
			break;
		case IDM_PREV:
			player->post_message(awkawk::previous);
			break;
		default:
			if(id < WM_USER)
			{
				FORWARD_WM_COMMAND(get_window(), id, control, event, &::DefWindowProc);
			}
		}
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
}

void player_window::onContextMenu(HWND, HWND, UINT x, UINT y)
{
	main_menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RECURSE, x, y, 0, get_window(), NULL);
	return;
}

void player_window::onDestroy(HWND)
{
	if(player->get_current_state() != awkawk::unloaded)
	{
		if(player->get_current_state() != awkawk::stopped)
		{
			player->send_message(awkawk::stop);
		}
		player->send_message(awkawk::unload);
		player->clear_files();
	}
	player->stop_rendering();
	::RevokeDragDrop(get_window());
	::PostQuitMessage(0);
}

void player_window::onNCPaint(HWND, HRGN)
{
}

BOOL player_window::onNCActivate(HWND, BOOL active, HWND, BOOL)
{
	return active == FALSE ? TRUE : FALSE;
}

UINT player_window::onNCCalcSize(HWND, BOOL calc_valid_rects, NCCALCSIZE_PARAMS* params)
{
	// we set our client area to take up the whole damn window
	if(calc_valid_rects == TRUE)
	{
		return WVR_REDRAW;
	}
	else
	{
		*reinterpret_cast<RECT*>(params) = get_window_rect();
		return 0;
	}
}

UINT player_window::onNCHitTest(HWND, int x, int y)
{
	if(player->is_fullscreen())
	{
		return HTCLIENT;
	}
	if(awkawk::free != player->get_window_size_mode())
	{
		return HTCLIENT;
	}
	RECT window_rect(get_window_rect());
	// we have an imaginary 4px border around the window which is used for our resize handles.
	// Virtually verything else is client area.  For dragging we trap the mouse movement directly, as we
	// have no title bar.
	static const int border_size(4);

	if(window_rect.left <= x && x <= (window_rect.left + border_size))
	{
		if(window_rect.top <= y && y <= (window_rect.top + border_size))
		{
			return HTTOPLEFT;
		}
		else if((window_rect.bottom - border_size) <= y && y <= window_rect.bottom)
		{
			return HTBOTTOMLEFT;
		}
		else
		{
			return HTLEFT;
		}
	}
	else if((window_rect.right - border_size) <= x && x <= window_rect.right)
	{
		if(window_rect.top <= y && y <= (window_rect.top + border_size))
		{
			return HTTOPRIGHT;
		}
		else if((window_rect.bottom - border_size) <= y && y <= window_rect.bottom)
		{
			return HTBOTTOMRIGHT;
		}
		else
		{
			return HTRIGHT;
		}
	}
	else if(window_rect.top <= y && y <= (window_rect.top + border_size))
	{
		return HTTOP;
	}
	else if((window_rect.bottom - border_size) <= y && y <= window_rect.bottom)
	{
		return HTBOTTOM;
	}
	else
	{
		return HTCLIENT;
	}
}

void player_window::update_last_mouse_move_time()
{
	::QueryPerformanceCounter(&last_mouse_move_time);
	if(player->is_fullscreen())
	{
		show_cursor();
	}
}

void player_window::onMouseMove(HWND, int x, int y, UINT)
{
	static int old_x(0);
	static int old_y(0);
	if(x == old_x && y == old_y)
	{
		return;
	}
	old_x = x;
	old_y = y;
	update_last_mouse_move_time();

	if(dragging)
	{
		POINT new_position = { GET_X_LPARAM(::GetMessagePos()), GET_Y_LPARAM(::GetMessagePos()) };
		RECT window_position(get_window_rect());
		send_message(WM_MOVING, 0, reinterpret_cast<LPARAM>(&window_position));
		move_window(window_position.left + new_position.x - old_position.x, window_position.top + new_position.y - old_position.y);
		old_position = new_position;
	}
	if(!tracking)
	{
		tracking = true;
		TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = get_window();
		::TrackMouseEvent(&tme);
	}
	POINT pt = { x, y };
	player->set_cursor_position(pt);
}

void player_window::resize_window(int new_width, int new_height)
{
	new_width &= ~1;
	new_height &= ~1;
	window::resize_window(new_width, new_height);
	//player->set_window_dimensions(get_window_size());
}

BOOL player_window::onSizing(HWND, UINT edge, RECT* coords)
{
	LONG width(coords->right - coords->left);
	LONG height(coords->bottom - coords->top);
	float ar(static_cast<float>(width) / static_cast<float>(height));
	// ar = w / h
	// w = h * ar
	// h = w / ar
	switch(edge)
	{
	case WMSZ_LEFT:
	case WMSZ_RIGHT:
		// if it's being made wider, preserve the width, adjust the height
		height = static_cast<LONG>(static_cast<float>(width) / player->get_aspect_ratio());
		coords->bottom = coords->top + height;
		break;
	case WMSZ_TOP:
	case WMSZ_BOTTOM:
		// if it's being made taller, preserve the height, adjust the width
		width = static_cast<LONG>(static_cast<float>(height) * player->get_aspect_ratio());
		coords->right = coords->left + width;
		break;
	// if the attempted AR > actual AR, preserve the width, adjust the height
	// else, preserve the height, adjust the width
	case WMSZ_TOPLEFT:
		if(ar > player->get_aspect_ratio())
		{
			height = static_cast<LONG>(static_cast<float>(width) / player->get_aspect_ratio());
			coords->top = coords->bottom - height;
		}
		else
		{
			width = static_cast<LONG>(static_cast<float>(height) * player->get_aspect_ratio());
			coords->left = coords->right - width;
		}
		break;
	case WMSZ_TOPRIGHT:
		if(ar > player->get_aspect_ratio())
		{
			height = static_cast<LONG>(static_cast<float>(width) / player->get_aspect_ratio());
			coords->top = coords->bottom - height;
		}
		else
		{
			width = static_cast<LONG>(static_cast<float>(height) * player->get_aspect_ratio());
			coords->right = coords->left + width;
		}
		break;
	case WMSZ_BOTTOMLEFT:
		if(ar > player->get_aspect_ratio())
		{
			height = static_cast<LONG>(static_cast<float>(width) / player->get_aspect_ratio());
			coords->bottom = coords->top + height;
		}
		else
		{
			width = static_cast<LONG>(static_cast<float>(height) * player->get_aspect_ratio());
			coords->left = coords->right - width;
		}
		break;
	case WMSZ_BOTTOMRIGHT:
		if(ar > player->get_aspect_ratio())
		{
			height = static_cast<LONG>(static_cast<float>(width) / player->get_aspect_ratio());
			coords->bottom = coords->top + height;
		}
		else
		{
			width = static_cast<LONG>(static_cast<float>(height) * player->get_aspect_ratio());
			coords->right = coords->left + width;
		}
		break;
	}
	return TRUE;
}

void player_window::onMouseLeave(HWND)
{
	POINT pt = { -1, -1 };
	player->set_cursor_position(pt);
	tracking = false;
}

void player_window::onSize(HWND, UINT size_type, int, int)
{
	switch(size_type)
	{
	case SIZE_MAXIMIZED:
		::SetTimer(get_window(), mouse_kill, 1000, NULL);
		break;
	case SIZE_MAXHIDE:
	case SIZE_MAXSHOW:
	case SIZE_MINIMIZED:
	case SIZE_RESTORED:
	default:
		::KillTimer(get_window(), mouse_kill);
		show_cursor();
	}
	player->set_window_dimensions(get_window_size());
}

void player_window::onEnterSizeMove(HWND)
{
}

void player_window::onExitSizeMove(HWND)
{
	player->set_window_dimensions(get_window_size());
}

void player_window::onGetMinMaxInfo(HWND, MINMAXINFO* min_max_info)
{
	HMONITOR monitor(::MonitorFromWindow(get_window(), MONITOR_DEFAULTTONEAREST));
	MONITORINFOEXW info;
	info.cbSize = sizeof(MONITORINFOEXW);
	::GetMonitorInfoW(monitor, &info);

	LONG working_width(info.rcWork.right - info.rcWork.left);
	LONG working_height(info.rcWork.bottom - info.rcWork.top);

	LONG monitor_width(info.rcMonitor.right - info.rcMonitor.left);
	LONG monitor_height(info.rcMonitor.bottom - info.rcMonitor.top);

	if(!player->is_fullscreen())
	{
		float ar(static_cast<float>(working_width) / static_cast<float>(working_height));

		if(ar > player->get_aspect_ratio()) // preserve height
		{
			working_width = static_cast<LONG>(static_cast<float>(working_height) * player->get_aspect_ratio());
		}
		else // preserve width
		{
			working_height = static_cast<LONG>(std::ceil(static_cast<float>(working_width) / player->get_aspect_ratio()));
		}
	}
	min_max_info->ptMaxSize.x = player->is_fullscreen() ? monitor_width : working_width;
	min_max_info->ptMaxSize.y = player->is_fullscreen() ? monitor_height : working_height;
	min_max_info->ptMaxTrackSize.x = player->is_fullscreen() ? monitor_width : working_width;
	min_max_info->ptMaxTrackSize.y = player->is_fullscreen() ? monitor_height : working_height;
	min_max_info->ptMinTrackSize.x = 160;
	min_max_info->ptMinTrackSize.y = static_cast<LONG>(static_cast<float>(min_max_info->ptMinTrackSize.x) / player->get_aspect_ratio());
	min_max_info->ptMaxPosition.x = 0;
	min_max_info->ptMaxPosition.y = 0;
}

void player_window::onLeftButtonDown(HWND, BOOL double_click, int, int, UINT)
{
	if(TRUE == double_click)
	{
		if(player->is_fullscreen())
		{
			send_message(WM_SYSCOMMAND, SC_RESTORE, 0);
		}
		else
		{
			send_message(WM_SYSCOMMAND, SC_MAXIMIZE, 0);
		}
		return;
	}
	if(!player->is_fullscreen())
	{
		POINT pt = { GET_X_LPARAM(::GetMessagePos()), GET_Y_LPARAM(::GetMessagePos()) };
		if(TRUE == ::DragDetect(get_window(), pt))
		{
			dragging = true;
			old_position.x = GET_X_LPARAM(::GetMessagePos());
			old_position.y = GET_Y_LPARAM(::GetMessagePos());
			::GetCursorPos(&old_position);
			RECT window_position(get_window_rect());
			snap_offset.x = old_position.x - window_position.left;
			snap_offset.y = old_position.y - window_position.top;
			::SetCapture(get_window());
			post_message(WM_ENTERSIZEMOVE, 0, 0);
		}
	}
}

void player_window::onLeftButtonUp(HWND, int, int, UINT)
{
	if(dragging)
	{
		dragging = false;
		::ReleaseCapture();
		post_message(WM_EXITSIZEMOVE, 0, 0);
	}
}

void player_window::onPaint(HWND)
{
	PAINTSTRUCT ps = {0};
	::BeginPaint(get_window(), &ps);
	::EndPaint(get_window(), &ps);
}

BOOL player_window::onMoving(HWND, RECT* coords)
{
	static const int snap_threshold(25);
	HMONITOR monitor(::MonitorFromRect(coords, MONITOR_DEFAULTTONEAREST));
	MONITORINFOEXW info;
	info.cbSize = sizeof(MONITORINFOEXW);
	::GetMonitorInfoW(monitor, &info);

	POINT cur_pos = { GET_X_LPARAM(::GetMessagePos()), GET_Y_LPARAM(::GetMessagePos()) };
	::OffsetRect(coords, cur_pos.x - (coords->left + snap_offset.x), cur_pos.y - (coords->top + snap_offset.y));

	if(std::abs(coords->left - info.rcWork.left) < snap_threshold)
	{
		::OffsetRect(coords, info.rcWork.left - coords->left, 0);
	}
	else if(std::abs(coords->right - info.rcWork.right) < snap_threshold)
	{
		::OffsetRect(coords, info.rcWork.right - coords->right, 0);
	}
	if(std::abs(coords->top - info.rcWork.top) < snap_threshold)
	{
		::OffsetRect(coords, 0, info.rcWork.top - coords->top);
	}
	else if(std::abs(coords->bottom - info.rcWork.bottom) < snap_threshold)
	{
		::OffsetRect(coords, 0, info.rcWork.bottom - coords->bottom);
	}
	return TRUE;
}

void player_window::onMove(HWND, int, int)
{
}

STDMETHODIMP player_window::drop_target::QueryInterface(const IID& iid, void** target)
{
	if(target == NULL)
	{
		return E_POINTER;
	}
	else if(iid == IID_IDropTarget)
	{
		*target = static_cast<IDropTarget*>(this);
		AddRef();
		return S_OK;
	}
	else
	{
		*target = NULL;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) player_window::drop_target::AddRef()
{
	return ::InterlockedIncrement(&ref_count);
}

STDMETHODIMP_(ULONG) player_window::drop_target::Release()
{
	ULONG ret(::InterlockedDecrement(&ref_count));
	if(ret == 0)
	{
		delete this;
	}
	return ret;
}

STDMETHODIMP player_window::drop_target::DragEnter(IDataObject* data_object, DWORD, POINTL, DWORD* allowed)
{
	drag_effect = *allowed & (is_droppable(data_object) ? DROPEFFECT_COPY : DROPEFFECT_NONE);
	return S_OK;
}

bool player_window::drop_target::is_droppable(IDataObjectPtr data_object)
{
	const _bstr_t file_name(get_file_name(data_object));
	if(file_name.length() == 0)
	{
		return false;
	}
	const DWORD attributes(::GetFileAttributesW(static_cast<const wchar_t*>(file_name)));
	if((attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		return false;
	}
	return true;
}

_bstr_t player_window::drop_target::get_file_name(IDataObjectPtr data_object)
{
	FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_ICON, -1, TYMED_HGLOBAL};
	if(data_object->QueryGetData(&fmt) != S_OK)
	{
		return L"";
	}
	STGMEDIUM medium = {0};
	if(data_object->GetData(&fmt, &medium) != S_OK)
	{
		return L"";
	}
	ON_BLOCK_EXIT(&::ReleaseStgMedium, &medium);
	const UINT file_count(::DragQueryFileW(reinterpret_cast<HDROP>(medium.hGlobal), 0xffffffff, NULL, 0));
	if(1 != file_count)
	{
		return L"";
	}
	const UINT buffer_size(::DragQueryFileW(reinterpret_cast<HDROP>(medium.hGlobal), 0, NULL, 0));
	boost::scoped_array<wchar_t> buffer(new wchar_t[buffer_size + 1]);
	::DragQueryFileW(reinterpret_cast<HDROP>(medium.hGlobal), 0, buffer.get(), buffer_size + 1);
	return buffer.get();
}

STDMETHODIMP player_window::drop_target::DragOver(DWORD, POINTL, DWORD* effect)
{
	*effect = drag_effect;
	return S_OK;
}

STDMETHODIMP player_window::drop_target::DragLeave()
{
	drag_effect = DROPEFFECT_NONE;
	return S_OK;
}

STDMETHODIMP player_window::drop_target::Drop(IDataObject* data_object, DWORD, POINTL, DWORD*)
{
	const _bstr_t file_name(get_file_name(data_object));
	if(file_name.length() > 0)
	{
		player_window->open_single_file(static_cast<const wchar_t*>(file_name));
	}
	return S_OK;
}
