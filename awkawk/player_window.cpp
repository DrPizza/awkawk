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
#include "load_url.h"
#include "player.h"
#include "resource.h"

player_window::player_window(Player* player_) : window(MAKEINTRESOURCE(IDC_PLAYER)), player(player_), tracking(false), dragging(false), in_size_move(false)
{
	boost::scoped_array<wchar_t> buffer(new wchar_t[256]);
	::LoadString(instance, IDS_APP_TITLE, buffer.get(), 256);
	app_title = buffer.get();
	::LoadString(instance, IDC_PLAYER, buffer.get(), 256);
	window_class_name = buffer.get();
	context_menu = ::LoadMenu(instance, MAKEINTRESOURCE(IDC_PLAYER));
	register_class(CS_DBLCLKS, ::LoadIcon(instance, MAKEINTRESOURCE(IDI_PLAYER)), ::LoadCursor(NULL, IDC_ARROW), static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)), NULL, window_class_name.c_str());
}

player_window::~player_window()
{
	::DestroyMenu(context_menu);
}

std::wstring player_window::get_local_path() const
{
	OPENFILENAME ofn = {0};
	boost::scoped_array<wchar_t> buffer(new wchar_t[0xffff]);
	buffer[0] = L'\0';

	static const wchar_t szFilter[] = L"Video Files (.asf, .avi, .mpg, .mpeg, .vob, .qt, .wmv, .mp4)\0*.ASF;*.AVI;*.MPG;*.MPEG;*.VOB;*.QT;*.WMV;*.MP4\0"
	                                  L"All Files (*.*)\0*.*\0";
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = get_window();
	ofn.hInstance = NULL;
	ofn.lpstrFilter = szFilter;
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
	
	if(::GetOpenFileName(&ofn) == TRUE)
	{
		return buffer.get();
	}
	return L"";
}

std::wstring player_window::get_remote_path() const
{
	load_url_dialogue dlg;
	switch(dlg.display_dialogue(MAKEINTRESOURCE(IDD_LOAD_URL), get_window()))
	{
	case load_url_dialogue::cancelled:
		return L"";
	case load_url_dialogue::url:
		return dlg.get_location().c_str();
	case load_url_dialogue::browse_local:
		return get_local_path();
	default:
		__assume(0);
	}
}

void player_window::create_window(int cmd_show)
{
	set_window(::CreateWindowEx(0, window_class_name.c_str(), app_title.c_str(), WS_CAPTION | WS_SYSMENU | WS_SIZEBOX, 100, 100, 640, 480, NULL, NULL, instance, this));
	//WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
	//placement.showCmd = cmd_show;
	//placement.ptMaxPosition.x = 0;
	//placement.ptMaxPosition.y = 0;
	//placement.ptMinPosition.x = 0;
	//placement.ptMinPosition.y = 0;
	//placement.rcNormalPosition.top = 100;
	//placement.rcNormalPosition.left = 100;
	//placement.rcNormalPosition.right = 740;
	//placement.rcNormalPosition.bottom = 580;
	//set_placement(&placement);
	SIZE sz = { 640, 480 };
	player->set_window_dimensions(sz);
	if(!get_window())
	{
		throw std::runtime_error("Could not create window");
	}
	drop_target* t(new drop_target(this));
	target.Attach(t, true);
	::RegisterDragDrop(get_window(), target);

	show_window(cmd_show);
}

LRESULT CALLBACK player_window::message_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	HANDLE_MSG(wnd, WM_COMMAND, onCommand);
	HANDLE_MSG(wnd, WM_CONTEXTMENU, onContextMenu);
	HANDLE_MSG(wnd, WM_DESTROY, onDestroy);
	HANDLE_MSG(wnd, WM_NCPAINT, onNCPaint);
	HANDLE_MSG(wnd, WM_NCCALCSIZE, onNCCalcSize);
	HANDLE_MSG(wnd, WM_NCACTIVATE, onNCActivate);
	HANDLE_MSG(wnd, WM_NCHITTEST, onNCHitTest);
	HANDLE_MSG(wnd, WM_MOUSEMOVE, onMouseMove);
	HANDLE_MSG(wnd, WM_SIZING, onSizing);
	HANDLE_MSG(wnd, WM_MOUSELEAVE, onMouseLeave);
	HANDLE_MSG(wnd, WM_SIZE, onSize);
	HANDLE_MSG(wnd, WM_ENTERSIZEMOVE, onEnterSizeMove);
	HANDLE_MSG(wnd, WM_EXITSIZEMOVE, onExitSizeMove);
	HANDLE_MSG(wnd, WM_LBUTTONDOWN, onLeftButtonDown);
	HANDLE_MSG(wnd, WM_LBUTTONDBLCLK, onLeftButtonDown);
	HANDLE_MSG(wnd, WM_LBUTTONUP, onLeftButtonUp);
	HANDLE_MSG(wnd, WM_MOVING, onMoving);
	HANDLE_MSG(wnd, WM_GETMINMAXINFO, onGetMinMaxInfo);
	HANDLE_MSG(wnd, WM_WINDOWPOSCHANGING, onWindowPosChanging);
	HANDLE_MSG(wnd, WM_WINDOWPOSCHANGED, onWindowPosChanged);

	default:
		return ::DefWindowProc(wnd, message, wParam, lParam);
	}
	return 0;
}

void player_window::onCommand(HWND wnd, int id, HWND control, UINT event)
{
	switch(id)
	{
	case IDM_LOAD_FILE:
	case IDM_LOAD_URL:
		{
			std::wstring path(id == IDM_LOAD_FILE ? get_local_path() : get_remote_path());
			if(path.size() > 0)
			{
				boost::scoped_array<wchar_t> buffer(new wchar_t[path.size() + 1]);
				std::memset(buffer.get(), 0, sizeof(wchar_t) * (path.size() + 1));
				std::copy(path.begin(), path.end(), buffer.get());
				::PathStripPath(buffer.get());
				::SetWindowText(wnd, buffer.get());
				if(player->get_state() != Player::unloaded)
				{
					player->stop();
					player->close();
				}
				player->clear_files();
				player->add_file(path);
				player->load();
				player->play();
			}
		}
		break;
	case IDM_PLAY:
		player->play();
		break;
	case IDM_PAUSE:
		player->pause();
		break;
	case IDM_STOP:
		player->stop();
		break;
	case IDM_NEXT:
		player->next();
		break;
	case IDM_PREV:
		player->prev();
		break;
	case IDM_PLAYMODE_NORMAL:
		player->set_playmode(Player::normal);
		break;
	case IDM_PLAYMODE_REPEATALL:
		player->set_playmode(Player::repeat_all);
		break;
	case IDM_PLAYMODE_REPEATTRACK:
		player->set_playmode(Player::repeat_single);
		break;
	case IDM_PLAYMODE_SHUFFLE:
		player->set_playmode(Player::shuffle);
		break;
	case IDM_CLOSE_FILE:
		{
			::SetWindowText(wnd, app_title.c_str());
			if(player->get_state() != Player::unloaded)
			{
				player->stop();
				player->close();
			}
		}
		break;
	case IDM_EXIT:
		if(player->get_state() != Player::unloaded)
		{
			player->stop();
			player->close();
			player->clear_files();
		}
		destroy_window();
		break;
	default:
		FORWARD_WM_COMMAND(wnd, id, control, event, ::DefWindowProc);
	}
}

void player_window::onContextMenu(HWND wnd, HWND hwndContext, UINT x, UINT y)
{
	HMENU main_menu(::GetSubMenu(context_menu, 0));
	HMENU play_menu(::GetSubMenu(main_menu, 2));
	HMENU playmode_menu(::GetSubMenu(main_menu, 3));
	switch(player->get_state())
	{
	case Player::unloaded:
		::EnableMenuItem(main_menu, IDM_LOAD_FILE, MF_ENABLED);
		::EnableMenuItem(main_menu, IDM_LOAD_URL, MF_ENABLED);
		::EnableMenuItem(main_menu, 2, MF_GRAYED | MF_DISABLED | MF_BYPOSITION);
			::EnableMenuItem(play_menu, IDM_PLAY, MF_GRAYED);
			::EnableMenuItem(play_menu, IDM_PAUSE, MF_GRAYED);
			::EnableMenuItem(play_menu, IDM_STOP, MF_GRAYED);
		::EnableMenuItem(main_menu, IDM_CLOSE_FILE, MF_GRAYED);
		break;
	case Player::stopped:
		::EnableMenuItem(main_menu, IDM_LOAD_FILE, MF_ENABLED);
		::EnableMenuItem(main_menu, IDM_LOAD_URL, MF_ENABLED);
		::EnableMenuItem(main_menu, 2, MF_ENABLED | MF_BYPOSITION);
			::EnableMenuItem(play_menu, IDM_PLAY, MF_ENABLED);
			::EnableMenuItem(play_menu, IDM_PAUSE, MF_GRAYED);
			::EnableMenuItem(play_menu, IDM_STOP, MF_GRAYED);
		::EnableMenuItem(main_menu, IDM_CLOSE_FILE, MF_ENABLED);
		break;
	case Player::playing:
		::EnableMenuItem(main_menu, IDM_LOAD_FILE, MF_ENABLED);
		::EnableMenuItem(main_menu, IDM_LOAD_URL, MF_ENABLED);
		::EnableMenuItem(main_menu, 2, MF_ENABLED | MF_BYPOSITION);
			::EnableMenuItem(play_menu, IDM_PLAY, MF_ENABLED);
			::EnableMenuItem(play_menu, IDM_PAUSE, MF_ENABLED);
			::EnableMenuItem(play_menu, IDM_STOP, MF_ENABLED);
		::EnableMenuItem(main_menu, IDM_CLOSE_FILE, MF_ENABLED);
		break;
	case Player::paused:
		::EnableMenuItem(main_menu, IDM_LOAD_FILE, MF_ENABLED);
		::EnableMenuItem(main_menu, IDM_LOAD_URL, MF_ENABLED);
		::EnableMenuItem(main_menu, 2, MF_ENABLED | MF_BYPOSITION);
			::EnableMenuItem(play_menu, IDM_PLAY, MF_ENABLED);
			::EnableMenuItem(play_menu, IDM_PAUSE, MF_ENABLED);
			::EnableMenuItem(play_menu, IDM_STOP, MF_ENABLED);
		::EnableMenuItem(main_menu, IDM_CLOSE_FILE, MF_ENABLED);
		break;
	}
	
	::EnableMenuItem(main_menu, IDM_LOAD_URL, MF_GRAYED);

	::EnableMenuItem(main_menu, 3, MF_ENABLED | MF_BYPOSITION);
		::EnableMenuItem(playmode_menu, IDM_PLAYMODE_NORMAL, MF_ENABLED);
		::CheckMenuItem(playmode_menu, IDM_PLAYMODE_NORMAL, player->get_playmode() == Player::normal ? MF_CHECKED : MF_UNCHECKED);
		::EnableMenuItem(playmode_menu, IDM_PLAYMODE_REPEATALL, MF_ENABLED);
		::CheckMenuItem(playmode_menu, IDM_PLAYMODE_REPEATALL, player->get_playmode() == Player::repeat_all ? MF_CHECKED : MF_UNCHECKED);
		::EnableMenuItem(playmode_menu, IDM_PLAYMODE_REPEATTRACK, MF_ENABLED);
		::CheckMenuItem(playmode_menu, IDM_PLAYMODE_REPEATTRACK, player->get_playmode() == Player::repeat_single ? MF_CHECKED : MF_UNCHECKED);
		//::EnableMenuItem(playmode_menu, IDM_PLAYMODE_SHUFFLE, MF_ENABLED);
		::EnableMenuItem(playmode_menu, IDM_PLAYMODE_SHUFFLE, MF_GRAYED);
		::CheckMenuItem(playmode_menu, IDM_PLAYMODE_SHUFFLE, player->get_playmode() == Player::shuffle ? MF_CHECKED : MF_UNCHECKED);
	::EnableMenuItem(main_menu, IDM_EXIT, MF_ENABLED);

	::TrackPopupMenu(main_menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RECURSE, x, y, 0, wnd, NULL);
}

void player_window::onDestroy(HWND wnd)
{
	player->close();
	::RevokeDragDrop(wnd);
	::PostQuitMessage(0);
}

void player_window::onNCPaint(HWND wnd, HRGN rgn)
{
}

BOOL player_window::onNCActivate(HWND wnd, BOOL active, HWND deactivated, BOOL minimized)
{
	return active == FALSE ? TRUE : FALSE;
}

UINT player_window::onNCCalcSize(HWND wnd, BOOL calcValidRects, NCCALCSIZE_PARAMS* params)
{
	// we set our client area to take up the whole damn window
	if(calcValidRects == TRUE)
	{
		params->rgrc[0] = get_window_rect();
	}
	else
	{
		*reinterpret_cast<RECT*>(params) = get_window_rect();
	}
	return 0;
}

UINT player_window::onNCHitTest(HWND wnd, int x, int y)
{
	if(player->is_fullscreen())
	{
		return HTCLIENT;
	}
	RECT wndRect(get_window_rect());
	// we have an imaginary 4px border around the window which is used for our resize handles.
	// Virtually verything else is client area.  For dragging we trap the mouse movement directly, as we
	// have no title bar.
	static const int borderSize(4);

	if(wndRect.left <= x && x <= (wndRect.left + borderSize))
	{
		if(wndRect.top <= y && y <= (wndRect.top + borderSize))
		{
			return HTTOPLEFT;
		}
		else if((wndRect.bottom - borderSize) <= y && y <= wndRect.bottom)
		{
			return HTBOTTOMLEFT;
		}
		else
		{
			return HTLEFT;
		}
	}
	else if((wndRect.right - borderSize) <= x && x <= wndRect.right)
	{
		if(wndRect.top <= y && y <= (wndRect.top + borderSize))
		{
			return HTTOPRIGHT;
		}
		else if((wndRect.bottom - borderSize) <= y && y <= wndRect.bottom)
		{
			return HTBOTTOMRIGHT;
		}
		else
		{
			return HTRIGHT;
		}
	}
	else if(wndRect.top <= y && y <= (wndRect.top + borderSize))
	{
		return HTTOP;
	}
	else if((wndRect.bottom - borderSize) <= y && y <= wndRect.bottom)
	{
		return HTBOTTOM;
	}
	else
	{
		return HTCLIENT;
	}
}

void player_window::onMouseMove(HWND wnd, int x, int y, UINT keyFlags)
{
	if(dragging)
	{
		POINT new_position = {0};
		::GetCursorPos(&new_position);
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
		tme.hwndTrack = wnd;
		::TrackMouseEvent(&tme);
	}
	POINT pt = { x, y };
	player->set_cursor_position(pt);
}

void player_window::resize_window(int newWidth, int newHeight)
{
	newWidth &= ~1;
	newHeight &= ~1;
	window::resize_window(newWidth, newHeight);
}

BOOL player_window::onSizing(HWND wnd, UINT edge, RECT* coords)
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

void player_window::onMouseLeave(HWND wnd)
{
	POINT pt = { -1, -1 };
	player->set_cursor_position(pt);
	tracking = false;
}

BOOL player_window::onWindowPosChanging(HWND wnd, WINDOWPOS* windowPos)
{
	return FALSE;
}

void player_window::onWindowPosChanged(HWND wnd, WINDOWPOS* windowPos)
{
}

void player_window::onSize(HWND wnd, UINT state, int x, int y)
{
	// fullscreen windows are allowed to be sized without regard for the AR
	if(player->is_fullscreen())
	{
		return;
	}
	// if I'm being resized by the window borders, WM_SIZING ensures the AR is preserved, and WM_EXITSIZEMOVE
	// ensures that the player gets notified
	if(in_size_move)
	{
		return;
	}
	float ar(static_cast<float>(x) / static_cast<float>(y));
	if(ar > player->get_aspect_ratio()) // too wide
	{
		x = static_cast<LONG>(static_cast<float>(y) * player->get_aspect_ratio());
	}
	else // too tall
	{
		y = static_cast<LONG>(static_cast<float>(x) / player->get_aspect_ratio());
	}

	post_message(WM_ENTERSIZEMOVE, 0, 0);
	resize_window(x, y);
	post_message(WM_EXITSIZEMOVE, 0, 0);
}

void player_window::onEnterSizeMove(HWND wnd)
{
	in_size_move = true;
}

void player_window::onExitSizeMove(HWND wnd)
{
	in_size_move = false;
	RECT window_dimensions(get_window_rect());
	SIZE sz = { window_dimensions.right - window_dimensions.left, window_dimensions.bottom - window_dimensions.top };
	player->set_window_dimensions(sz);
}

void player_window::onGetMinMaxInfo(HWND wnd, MINMAXINFO* minMaxInfo)
{
	HMONITOR monitor(::MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST));
	MONITORINFOEX info;
	info.cbSize = sizeof(MONITORINFOEX);
	::GetMonitorInfo(monitor, &info);

	LONG working_width(info.rcWork.right - info.rcWork.left);
	LONG working_height(info.rcWork.bottom - info.rcWork.top);

	LONG monitor_width(info.rcMonitor.right - info.rcMonitor.left);
	LONG monitor_height(info.rcMonitor.bottom - info.rcMonitor.top);

	if(!player->is_fullscreen())
	{
		double ar(static_cast<double>(working_width) / static_cast<double>(working_height));

		if(ar > player->get_aspect_ratio()) // preserve height
		{
			working_width = static_cast<LONG>(static_cast<double>(working_height) * player->get_aspect_ratio());
		}
		else // preserve width
		{
			working_height = static_cast<LONG>(std::ceil(static_cast<double>(working_width) / player->get_aspect_ratio()));
		}
	}
	minMaxInfo->ptMaxSize.x = player->is_fullscreen() ? monitor_width : working_width;
	minMaxInfo->ptMaxSize.y = player->is_fullscreen() ? monitor_height : working_height;
	minMaxInfo->ptMaxTrackSize.x = player->is_fullscreen() ? monitor_width : working_width;
	minMaxInfo->ptMaxTrackSize.y = player->is_fullscreen() ? monitor_height : working_height;
	minMaxInfo->ptMinTrackSize.x = 160;
	minMaxInfo->ptMinTrackSize.y = static_cast<LONG>(static_cast<double>(minMaxInfo->ptMinTrackSize.x) / player->get_aspect_ratio());
	minMaxInfo->ptMaxPosition.x = 0;
	minMaxInfo->ptMaxPosition.y = 0;
}

void player_window::onLeftButtonDown(HWND wnd, BOOL doubleClick, int x, int y, UINT keyFlags)
{
	if(TRUE == doubleClick)
	{
		player->toggle_fullscreen();
	}

	dragging = true;
	::GetCursorPos(&old_position);
	RECT window_position(get_window_rect());
	snap_offset.x = old_position.x - window_position.left;
	snap_offset.y = old_position.y - window_position.top;
	::SetCapture(wnd);
	post_message(WM_ENTERSIZEMOVE, 0, 0);
}

void player_window::onLeftButtonUp(HWND wnd, int x, int y, UINT keyFlags)
{
	if(dragging)
	{
		dragging = false;
		::ReleaseCapture();
		post_message(WM_EXITSIZEMOVE, 0, 0);
	}
}

BOOL player_window::onMoving(HWND wnd, RECT* coords)
{
	static const int snap_threshold(25);
	HMONITOR monitor(::MonitorFromRect(coords, MONITOR_DEFAULTTONEAREST));
	MONITORINFOEX info;
	info.cbSize = sizeof(MONITORINFOEX);
	::GetMonitorInfo(monitor, &info);

	POINT cur_pos;
	::GetCursorPos(&cur_pos);
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

STDMETHODIMP player_window::drop_target::QueryInterface(const IID& iid, void** ppvObject)
{
	if(ppvObject == NULL)
	{
		return E_POINTER;
	}
	else if(iid == IID_IDropTarget)
	{
		*ppvObject = static_cast<IDropTarget*>(this);
		AddRef();
		return S_OK;
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

STDMETHODIMP player_window::drop_target::DragEnter(IDataObject* dataObject, DWORD keyState, POINTL pt, DWORD* allowed)
{
	drag_effect = *allowed & (is_droppable(dataObject) ? DROPEFFECT_COPY : DROPEFFECT_NONE);
	return S_OK;
}

bool player_window::drop_target::is_droppable(IDataObjectPtr dataObject)
{
	const _bstr_t fileName(get_file_name(dataObject));
	if(fileName.length() == 0)
	{
		return false;
	}
	const DWORD attributes(::GetFileAttributes(static_cast<const wchar_t*>(fileName)));
	if((attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		return false;
	}
	return true;
}

_bstr_t player_window::drop_target::get_file_name(IDataObjectPtr dataObject)
{
	FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_ICON, -1, TYMED_HGLOBAL};
	if(dataObject->QueryGetData(&fmt) != S_OK)
	{
		return L"";
	}
	STGMEDIUM medium = {0};
	if(dataObject->GetData(&fmt, &medium) != S_OK)
	{
		return L"";
	}
	ON_BLOCK_EXIT(&::ReleaseStgMedium, &medium);
	const UINT fileCount(::DragQueryFile(reinterpret_cast<HDROP>(medium.hGlobal), 0xffffffff, NULL, 0));
	if(1 != fileCount)
	{
		return L"";
	}
	const UINT bufferSize(::DragQueryFile(reinterpret_cast<HDROP>(medium.hGlobal), 0, NULL, 0));
	boost::scoped_array<wchar_t> buffer(new wchar_t[bufferSize + 1]);
	::DragQueryFile(reinterpret_cast<HDROP>(medium.hGlobal), 0, buffer.get(), bufferSize + 1);
	return buffer.get();
}

STDMETHODIMP player_window::drop_target::DragOver(DWORD keyState, POINTL pt, DWORD* effect)
{
	*effect = drag_effect;
	return S_OK;
}

STDMETHODIMP player_window::drop_target::DragLeave()
{
	drag_effect = DROPEFFECT_NONE;
	return S_OK;
}

STDMETHODIMP player_window::drop_target::Drop(IDataObject* dataObject, DWORD keyState, POINTL pt, DWORD* effect)
{
	const _bstr_t fileName(get_file_name(dataObject));
	if(fileName.length() > 0)
	{
		if(player_window->player->get_state() != Player::unloaded)
		{
			player_window->player->stop();
			player_window->player->close();
			player_window->player->clear_files();
		}
		player_window->player->add_file(static_cast<const wchar_t*>(fileName));
		player_window->player->load();
		player_window->player->play();
	}
	return S_OK;
}
