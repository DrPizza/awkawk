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
#define NTDDI_VERSION NTDDI_LONGHORN
#define STRICT
#pragma warning(disable:4995)
#pragma warning(disable:4996)

#include <objbase.h>
#include <windows.h>
#include <windowsx.h>

#include <set>

#include "util.h"

// why doesn't windowsx.h include these?

// BOOL OnSizing(HWND hwnd, UINT edge, RECT* coords)
#define HANDLE_WM_SIZING(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(BOOL)(fn)((hwnd), (UINT)wParam, (RECT*)(lParam))
#define FORWARD_WM_SIZING(hwnd, edge, coords, fn) \
    (BOOL)(DWORD)(fn)((hwnd), WM_SIZING, (WPARAM)(edge), (LPARAM)(RECT*)(coords))

// BOOL OnMoving(HWND hwnd, RECT* coords)
#define HANDLE_WM_MOVING(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(BOOL)(fn)((hwnd), (RECT*)(lParam))
#define FORWARD_WM_MOVING(hwnd, edge, coords, fn) \
    (BOOL)(DWORD)(fn)((hwnd), WM_MOVING, 0, (LPARAM)(RECT*)(coords))

// BOOL OnMouseLeave(HWND hwnd)
#define HANDLE_WM_MOUSELEAVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_MOUSELEAVE(hwnd, fn) \
    (void)(DWORD)(fn)((hwnd), WM_MOUSELEAVE, 0UL, 0L)

// BOOL OnNcMouseLeave(HWND hwnd)
#define HANDLE_WM_NCMOUSELEAVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_NCMOUSELEAVE(hwnd, fn) \
    (void)(DWORD)(fn)((hwnd), WM_NCMOUSELEAVE, 0UL, 0L)

// void OnEnterSizeMove(HWND hwnd)
#define HANDLE_WM_ENTERSIZEMOVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_ENTERSIZEMOVE(hwnd, fn) \
    (void)(DWORD)(fn)((hwnd), WM_ENTERSIZEMOVE, 0UL, 0L)

// void OnExitSizeMove(HWND hwnd)
#define HANDLE_WM_EXITSIZEMOVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#define FORWARD_WM_EXITSIZEMOVE(hwnd, fn) \
    (void)(DWORD)(fn)((hwnd), WM_EXITSIZEMOVE, 0UL, 0L)

inline void print_message(UINT message)
{
	switch(message)
	{
	case WM_NULL: dout << "WM_NULL" << std::endl; break;
	case WM_CREATE: dout << "WM_CREATE" << std::endl; break;
	case WM_DESTROY: dout << "WM_DESTROY" << std::endl; break;
	case WM_MOVE: dout << "WM_MOVE" << std::endl; break;
	case WM_SIZE: dout << "WM_SIZE" << std::endl; break;
	case WM_ACTIVATE: dout << "WM_ACTIVATE" << std::endl; break;
	case WM_SETFOCUS: dout << "WM_SETFOCUS" << std::endl; break;
	case WM_KILLFOCUS: dout << "WM_KILLFOCUS" << std::endl; break;
	case WM_ENABLE: dout << "WM_ENABLE" << std::endl; break;
	case WM_SETREDRAW: dout << "WM_SETREDRAW" << std::endl; break;
	case WM_SETTEXT: dout << "WM_SETTEXT" << std::endl; break;
	case WM_GETTEXT: dout << "WM_GETTEXT" << std::endl; break;
	case WM_GETTEXTLENGTH: dout << "WM_GETTEXTLENGTH" << std::endl; break;
	case WM_PAINT: dout << "WM_PAINT" << std::endl; break;
	case WM_CLOSE: dout << "WM_CLOSE" << std::endl; break;
	case WM_QUERYENDSESSION: dout << "WM_QUERYENDSESSION" << std::endl; break;
	case WM_QUERYOPEN: dout << "WM_QUERYOPEN" << std::endl; break;
	case WM_ENDSESSION: dout << "WM_ENDSESSION" << std::endl; break;
	case WM_QUIT: dout << "WM_QUIT" << std::endl; break;
	case WM_ERASEBKGND: dout << "WM_ERASEBKGND" << std::endl; break;
	case WM_SYSCOLORCHANGE: dout << "WM_SYSCOLORCHANGE" << std::endl; break;
	case WM_SHOWWINDOW: dout << "WM_SHOWWINDOW" << std::endl; break;
	case WM_SETTINGCHANGE: dout << "WM_SETTINGCHANGE" << std::endl; break;
	case WM_DEVMODECHANGE: dout << "WM_DEVMODECHANGE" << std::endl; break;
	case WM_ACTIVATEAPP: dout << "WM_ACTIVATEAPP" << std::endl; break;
	case WM_FONTCHANGE: dout << "WM_FONTCHANGE" << std::endl; break;
	case WM_TIMECHANGE: dout << "WM_TIMECHANGE" << std::endl; break;
	case WM_CANCELMODE: dout << "WM_CANCELMODE" << std::endl; break;
	case WM_SETCURSOR: dout << "WM_SETCURSOR" << std::endl; break;
	case WM_MOUSEACTIVATE: dout << "WM_MOUSEACTIVATE" << std::endl; break;
	case WM_CHILDACTIVATE: dout << "WM_CHILDACTIVATE" << std::endl; break;
	case WM_QUEUESYNC: dout << "WM_QUEUESYNC" << std::endl; break;
	case WM_GETMINMAXINFO: dout << "WM_GETMINMAXINFO" << std::endl; break;
	case WM_PAINTICON: dout << "WM_PAINTICON" << std::endl; break;
	case WM_ICONERASEBKGND: dout << "WM_ICONERASEBKGND" << std::endl; break;
	case WM_NEXTDLGCTL: dout << "WM_NEXTDLGCTL" << std::endl; break;
	case WM_SPOOLERSTATUS: dout << "WM_SPOOLERSTATUS" << std::endl; break;
	case WM_DRAWITEM: dout << "WM_DRAWITEM" << std::endl; break;
	case WM_MEASUREITEM: dout << "WM_MEASUREITEM" << std::endl; break;
	case WM_DELETEITEM: dout << "WM_DELETEITEM" << std::endl; break;
	case WM_VKEYTOITEM: dout << "WM_VKEYTOITEM" << std::endl; break;
	case WM_CHARTOITEM: dout << "WM_CHARTOITEM" << std::endl; break;
	case WM_SETFONT: dout << "WM_SETFONT" << std::endl; break;
	case WM_GETFONT: dout << "WM_GETFONT" << std::endl; break;
	case WM_SETHOTKEY: dout << "WM_SETHOTKEY" << std::endl; break;
	case WM_GETHOTKEY: dout << "WM_GETHOTKEY" << std::endl; break;
	case WM_QUERYDRAGICON: dout << "WM_QUERYDRAGICON" << std::endl; break;
	case WM_COMPAREITEM: dout << "WM_COMPAREITEM" << std::endl; break;
	case WM_GETOBJECT: dout << "WM_GETOBJECT" << std::endl; break;
	case WM_COMPACTING: dout << "WM_COMPACTING" << std::endl; break;
	case WM_COMMNOTIFY: dout << "WM_COMMNOTIFY" << std::endl; break;
	case WM_WINDOWPOSCHANGING: dout << "WM_WINDOWPOSCHANGING" << std::endl; break;
	case WM_WINDOWPOSCHANGED: dout << "WM_WINDOWPOSCHANGED" << std::endl; break;
	case WM_POWER: dout << "WM_POWER" << std::endl; break;
	case WM_COPYDATA: dout << "WM_COPYDATA" << std::endl; break;
	case WM_CANCELJOURNAL: dout << "WM_CANCELJOURNAL" << std::endl; break;
	case WM_NOTIFY: dout << "WM_NOTIFY" << std::endl; break;
	case WM_INPUTLANGCHANGEREQUEST: dout << "WM_INPUTLANGCHANGEREQUEST" << std::endl; break;
	case WM_INPUTLANGCHANGE: dout << "WM_INPUTLANGCHANGE" << std::endl; break;
	case WM_TCARD: dout << "WM_TCARD" << std::endl; break;
	case WM_HELP: dout << "WM_HELP" << std::endl; break;
	case WM_USERCHANGED: dout << "WM_USERCHANGED" << std::endl; break;
	case WM_NOTIFYFORMAT: dout << "WM_NOTIFYFORMAT" << std::endl; break;
	case WM_CONTEXTMENU: dout << "WM_CONTEXTMENU" << std::endl; break;
	case WM_STYLECHANGING: dout << "WM_STYLECHANGING" << std::endl; break;
	case WM_STYLECHANGED: dout << "WM_STYLECHANGED" << std::endl; break;
	case WM_DISPLAYCHANGE: dout << "WM_DISPLAYCHANGE" << std::endl; break;
	case WM_GETICON: dout << "WM_GETICON" << std::endl; break;
	case WM_SETICON: dout << "WM_SETICON" << std::endl; break;
	case WM_NCCREATE: dout << "WM_NCCREATE" << std::endl; break;
	case WM_NCDESTROY: dout << "WM_NCDESTROY" << std::endl; break;
	case WM_NCCALCSIZE: dout << "WM_NCCALCSIZE" << std::endl; break;
	case WM_NCHITTEST: dout << "WM_NCHITTEST" << std::endl; break;
	case WM_NCPAINT: dout << "WM_NCPAINT" << std::endl; break;
	case WM_NCACTIVATE: dout << "WM_NCACTIVATE" << std::endl; break;
	case WM_GETDLGCODE: dout << "WM_GETDLGCODE" << std::endl; break;
	case WM_SYNCPAINT: dout << "WM_SYNCPAINT" << std::endl; break;
	case WM_NCMOUSEMOVE: dout << "WM_NCMOUSEMOVE" << std::endl; break;
	case WM_NCLBUTTONDOWN: dout << "WM_NCLBUTTONDOWN" << std::endl; break;
	case WM_NCLBUTTONUP: dout << "WM_NCLBUTTONUP" << std::endl; break;
	case WM_NCLBUTTONDBLCLK: dout << "WM_NCLBUTTONDBLCLK" << std::endl; break;
	case WM_NCRBUTTONDOWN: dout << "WM_NCRBUTTONDOWN" << std::endl; break;
	case WM_NCRBUTTONUP: dout << "WM_NCRBUTTONUP" << std::endl; break;
	case WM_NCRBUTTONDBLCLK: dout << "WM_NCRBUTTONDBLCLK" << std::endl; break;
	case WM_NCMBUTTONDOWN: dout << "WM_NCMBUTTONDOWN" << std::endl; break;
	case WM_NCMBUTTONUP: dout << "WM_NCMBUTTONUP" << std::endl; break;
	case WM_NCMBUTTONDBLCLK: dout << "WM_NCMBUTTONDBLCLK" << std::endl; break;
	case WM_NCXBUTTONDOWN: dout << "WM_NCXBUTTONDOWN" << std::endl; break;
	case WM_NCXBUTTONUP: dout << "WM_NCXBUTTONUP" << std::endl; break;
	case WM_NCXBUTTONDBLCLK: dout << "WM_NCXBUTTONDBLCLK" << std::endl; break;
	case WM_INPUT_DEVICE_CHANGE: dout << "WM_INPUT_DEVICE_CHANGE" << std::endl; break;
	case WM_INPUT: dout << "WM_INPUT" << std::endl; break;
	case WM_KEYDOWN: dout << "WM_KEYDOWN" << std::endl; break;
	case WM_KEYUP: dout << "WM_KEYUP" << std::endl; break;
	case WM_CHAR: dout << "WM_CHAR" << std::endl; break;
	case WM_DEADCHAR: dout << "WM_DEADCHAR" << std::endl; break;
	case WM_SYSKEYDOWN: dout << "WM_SYSKEYDOWN" << std::endl; break;
	case WM_SYSKEYUP: dout << "WM_SYSKEYUP" << std::endl; break;
	case WM_SYSCHAR: dout << "WM_SYSCHAR" << std::endl; break;
	case WM_SYSDEADCHAR: dout << "WM_SYSDEADCHAR" << std::endl; break;
	case WM_UNICHAR: dout << "WM_UNICHAR" << std::endl; break;
	case WM_IME_STARTCOMPOSITION: dout << "WM_IME_STARTCOMPOSITION" << std::endl; break;
	case WM_IME_ENDCOMPOSITION: dout << "WM_IME_ENDCOMPOSITION" << std::endl; break;
	case WM_IME_COMPOSITION: dout << "WM_IME_COMPOSITION" << std::endl; break;
	case WM_INITDIALOG: dout << "WM_INITDIALOG" << std::endl; break;
	case WM_COMMAND: dout << "WM_COMMAND" << std::endl; break;
	case WM_SYSCOMMAND: dout << "WM_SYSCOMMAND" << std::endl; break;
	case WM_TIMER: dout << "WM_TIMER" << std::endl; break;
	case WM_HSCROLL: dout << "WM_HSCROLL" << std::endl; break;
	case WM_VSCROLL: dout << "WM_VSCROLL" << std::endl; break;
	case WM_INITMENU: dout << "WM_INITMENU" << std::endl; break;
	case WM_INITMENUPOPUP: dout << "WM_INITMENUPOPUP" << std::endl; break;
	case WM_MENUSELECT: dout << "WM_MENUSELECT" << std::endl; break;
	case WM_MENUCHAR: dout << "WM_MENUCHAR" << std::endl; break;
	case WM_ENTERIDLE: dout << "WM_ENTERIDLE" << std::endl; break;
	case WM_MENURBUTTONUP: dout << "WM_MENURBUTTONUP" << std::endl; break;
	case WM_MENUDRAG: dout << "WM_MENUDRAG" << std::endl; break;
	case WM_MENUGETOBJECT: dout << "WM_MENUGETOBJECT" << std::endl; break;
	case WM_UNINITMENUPOPUP: dout << "WM_UNINITMENUPOPUP" << std::endl; break;
	case WM_MENUCOMMAND: dout << "WM_MENUCOMMAND" << std::endl; break;
	case WM_CHANGEUISTATE: dout << "WM_CHANGEUISTATE" << std::endl; break;
	case WM_UPDATEUISTATE: dout << "WM_UPDATEUISTATE" << std::endl; break;
	case WM_QUERYUISTATE: dout << "WM_QUERYUISTATE" << std::endl; break;
	case WM_CTLCOLORMSGBOX: dout << "WM_CTLCOLORMSGBOX" << std::endl; break;
	case WM_CTLCOLOREDIT: dout << "WM_CTLCOLOREDIT" << std::endl; break;
	case WM_CTLCOLORLISTBOX: dout << "WM_CTLCOLORLISTBOX" << std::endl; break;
	case WM_CTLCOLORBTN: dout << "WM_CTLCOLORBTN" << std::endl; break;
	case WM_CTLCOLORDLG: dout << "WM_CTLCOLORDLG" << std::endl; break;
	case WM_CTLCOLORSCROLLBAR: dout << "WM_CTLCOLORSCROLLBAR" << std::endl; break;
	case WM_CTLCOLORSTATIC: dout << "WM_CTLCOLORSTATIC" << std::endl; break;
	case MN_GETHMENU: dout << "MN_GETHMENU" << std::endl; break;
	case WM_MOUSEMOVE: dout << "WM_MOUSEMOVE" << std::endl; break;
	case WM_LBUTTONDOWN: dout << "WM_LBUTTONDOWN" << std::endl; break;
	case WM_LBUTTONUP: dout << "WM_LBUTTONUP" << std::endl; break;
	case WM_LBUTTONDBLCLK: dout << "WM_LBUTTONDBLCLK" << std::endl; break;
	case WM_RBUTTONDOWN: dout << "WM_RBUTTONDOWN" << std::endl; break;
	case WM_RBUTTONUP: dout << "WM_RBUTTONUP" << std::endl; break;
	case WM_RBUTTONDBLCLK: dout << "WM_RBUTTONDBLCLK" << std::endl; break;
	case WM_MBUTTONDOWN: dout << "WM_MBUTTONDOWN" << std::endl; break;
	case WM_MBUTTONUP: dout << "WM_MBUTTONUP" << std::endl; break;
	case WM_MBUTTONDBLCLK: dout << "WM_MBUTTONDBLCLK" << std::endl; break;
	case WM_MOUSEWHEEL: dout << "WM_MOUSEWHEEL" << std::endl; break;
	case WM_XBUTTONDOWN: dout << "WM_XBUTTONDOWN" << std::endl; break;
	case WM_XBUTTONUP: dout << "WM_XBUTTONUP" << std::endl; break;
	case WM_XBUTTONDBLCLK: dout << "WM_XBUTTONDBLCLK" << std::endl; break;
	case WM_MOUSEHWHEEL: dout << "WM_MOUSEHWHEEL" << std::endl; break;
	case WM_PARENTNOTIFY: dout << "WM_PARENTNOTIFY" << std::endl; break;
	case WM_ENTERMENULOOP: dout << "WM_ENTERMENULOOP" << std::endl; break;
	case WM_EXITMENULOOP: dout << "WM_EXITMENULOOP" << std::endl; break;
	case WM_NEXTMENU: dout << "WM_NEXTMENU" << std::endl; break;
	case WM_SIZING: dout << "WM_SIZING" << std::endl; break;
	case WM_CAPTURECHANGED: dout << "WM_CAPTURECHANGED" << std::endl; break;
	case WM_MOVING: dout << "WM_MOVING" << std::endl; break;
	case WM_POWERBROADCAST: dout << "WM_POWERBROADCAST" << std::endl; break;
	case WM_DEVICECHANGE: dout << "WM_DEVICECHANGE" << std::endl; break;
	case WM_MDICREATE: dout << "WM_MDICREATE" << std::endl; break;
	case WM_MDIDESTROY: dout << "WM_MDIDESTROY" << std::endl; break;
	case WM_MDIACTIVATE: dout << "WM_MDIACTIVATE" << std::endl; break;
	case WM_MDIRESTORE: dout << "WM_MDIRESTORE" << std::endl; break;
	case WM_MDINEXT: dout << "WM_MDINEXT" << std::endl; break;
	case WM_MDIMAXIMIZE: dout << "WM_MDIMAXIMIZE" << std::endl; break;
	case WM_MDITILE: dout << "WM_MDITILE" << std::endl; break;
	case WM_MDICASCADE: dout << "WM_MDICASCADE" << std::endl; break;
	case WM_MDIICONARRANGE: dout << "WM_MDIICONARRANGE" << std::endl; break;
	case WM_MDIGETACTIVE: dout << "WM_MDIGETACTIVE" << std::endl; break;
	case WM_MDISETMENU: dout << "WM_MDISETMENU" << std::endl; break;
	case WM_ENTERSIZEMOVE: dout << "WM_ENTERSIZEMOVE" << std::endl; break;
	case WM_EXITSIZEMOVE: dout << "WM_EXITSIZEMOVE" << std::endl; break;
	case WM_DROPFILES: dout << "WM_DROPFILES" << std::endl; break;
	case WM_MDIREFRESHMENU: dout << "WM_MDIREFRESHMENU" << std::endl; break;
	case WM_IME_SETCONTEXT: dout << "WM_IME_SETCONTEXT" << std::endl; break;
	case WM_IME_NOTIFY: dout << "WM_IME_NOTIFY" << std::endl; break;
	case WM_IME_CONTROL: dout << "WM_IME_CONTROL" << std::endl; break;
	case WM_IME_COMPOSITIONFULL: dout << "WM_IME_COMPOSITIONFULL" << std::endl; break;
	case WM_IME_SELECT: dout << "WM_IME_SELECT" << std::endl; break;
	case WM_IME_CHAR: dout << "WM_IME_CHAR" << std::endl; break;
	case WM_IME_REQUEST: dout << "WM_IME_REQUEST" << std::endl; break;
	case WM_IME_KEYDOWN: dout << "WM_IME_KEYDOWN" << std::endl; break;
	case WM_IME_KEYUP: dout << "WM_IME_KEYUP" << std::endl; break;
	case WM_MOUSEHOVER: dout << "WM_MOUSEHOVER" << std::endl; break;
	case WM_MOUSELEAVE: dout << "WM_MOUSELEAVE" << std::endl; break;
	case WM_NCMOUSEHOVER: dout << "WM_NCMOUSEHOVER" << std::endl; break;
	case WM_NCMOUSELEAVE: dout << "WM_NCMOUSELEAVE" << std::endl; break;
	case WM_WTSSESSION_CHANGE: dout << "WM_WTSSESSION_CHANGE" << std::endl; break;
	case WM_TABLET_FIRST: dout << "WM_TABLET_FIRST" << std::endl; break;
	case WM_TABLET_LAST: dout << "WM_TABLET_LAST" << std::endl; break;
	case WM_CUT: dout << "WM_CUT" << std::endl; break;
	case WM_COPY: dout << "WM_COPY" << std::endl; break;
	case WM_PASTE: dout << "WM_PASTE" << std::endl; break;
	case WM_CLEAR: dout << "WM_CLEAR" << std::endl; break;
	case WM_UNDO: dout << "WM_UNDO" << std::endl; break;
	case WM_RENDERFORMAT: dout << "WM_RENDERFORMAT" << std::endl; break;
	case WM_RENDERALLFORMATS: dout << "WM_RENDERALLFORMATS" << std::endl; break;
	case WM_DESTROYCLIPBOARD: dout << "WM_DESTROYCLIPBOARD" << std::endl; break;
	case WM_DRAWCLIPBOARD: dout << "WM_DRAWCLIPBOARD" << std::endl; break;
	case WM_PAINTCLIPBOARD: dout << "WM_PAINTCLIPBOARD" << std::endl; break;
	case WM_VSCROLLCLIPBOARD: dout << "WM_VSCROLLCLIPBOARD" << std::endl; break;
	case WM_SIZECLIPBOARD: dout << "WM_SIZECLIPBOARD" << std::endl; break;
	case WM_ASKCBFORMATNAME: dout << "WM_ASKCBFORMATNAME" << std::endl; break;
	case WM_CHANGECBCHAIN: dout << "WM_CHANGECBCHAIN" << std::endl; break;
	case WM_HSCROLLCLIPBOARD: dout << "WM_HSCROLLCLIPBOARD" << std::endl; break;
	case WM_QUERYNEWPALETTE: dout << "WM_QUERYNEWPALETTE" << std::endl; break;
	case WM_PALETTEISCHANGING: dout << "WM_PALETTEISCHANGING" << std::endl; break;
	case WM_PALETTECHANGED: dout << "WM_PALETTECHANGED" << std::endl; break;
	case WM_HOTKEY: dout << "WM_HOTKEY" << std::endl; break;
	case WM_PRINT: dout << "WM_PRINT" << std::endl; break;
	case WM_PRINTCLIENT: dout << "WM_PRINTCLIENT" << std::endl; break;
	case WM_APPCOMMAND: dout << "WM_APPCOMMAND" << std::endl; break;
	case WM_THEMECHANGED: dout << "WM_THEMECHANGED" << std::endl; break;
	case WM_CLIPBOARDUPDATE: dout << "WM_CLIPBOARDUPDATE" << std::endl; break;
	case WM_DWMCOMPOSITIONCHANGED: dout << "WM_DWMCOMPOSITIONCHANGED" << std::endl; break;
	case WM_DWMNCRENDERINGCHANGED: dout << "WM_DWMNCRENDERINGCHANGED" << std::endl; break;
	case WM_DWMCOLORIZATIONCOLORCHANGED: dout << "WM_DWMCOLORIZATIONCOLORCHANGED" << std::endl; break;
	case WM_DWMWINDOWMAXIMIZEDCHANGE: dout << "WM_DWMWINDOWMAXIMIZEDCHANGE" << std::endl; break;
	case WM_GETTITLEBARINFOEX: dout << "WM_GETTITLEBARINFOEX" << std::endl; break;
	case WM_HANDHELDFIRST: dout << "WM_HANDHELDFIRST" << std::endl; break;
	case WM_HANDHELDLAST: dout << "WM_HANDHELDLAST" << std::endl; break;
	case WM_AFXFIRST: dout << "WM_AFXFIRST" << std::endl; break;
	case WM_AFXLAST: dout << "WM_AFXLAST" << std::endl; break;
	case WM_PENWINFIRST: dout << "WM_PENWINFIRST" << std::endl; break;
	case WM_PENWINLAST: dout << "WM_PENWINLAST" << std::endl; break;
	case WM_APP: dout << "WM_APP" << std::endl; break;
	case WM_USER: dout << "WM_USER" << std::endl; break;
	default: dout << "Unknown message: " << std::hex << message << std::endl;
	}
}

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
	window(const wchar_t* window_class_name, UINT style, HICON icon, HCURSOR cursor, HBRUSH background, const wchar_t* menu_name, const wchar_t* accelerators_id) : message_handler(NULL),
	                                                                                                                                                                instance(::GetModuleHandle(NULL)),
	                                                                                                                                                                accelerator_table(accelerators_id != NULL ? ::LoadAcceleratorsW(instance, accelerators_id) : NULL),
	                                                                                                                                                                registered(true)

	{
		window_class.cbSize = sizeof(WNDCLASSEXW);
		window_class.style = style;
		window_class.lpfnWndProc = &window::message_proc_helper;
		window_class.cbClsExtra = 0;
		window_class.cbWndExtra = sizeof(window*);
		window_class.hInstance = instance;
		window_class.hIcon = icon;
		window_class.hCursor = cursor;
		window_class.hbrBackground = background;
		window_class.lpszMenuName = menu_name;
		window_class.lpszClassName = window_class_name;
		window_class.hIconSm = NULL;
		register_class();
	}

	virtual ~window()
	{
		if(accelerator_table != NULL)
		{
			::DestroyAcceleratorTable(accelerator_table);
		}
		unregister_class();
	}

	HWND create_window(DWORD extended_style, const wchar_t* window_name, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, void* param)
	{
		std::auto_ptr<std::pair<void*, void*> > ptrs(new std::pair<void*, void*>(this, param));
		set_window(::CreateWindowExW(extended_style, window_class.lpszClassName, window_name, style, x, y, width, height, parent, menu, instance, ptrs.get()));
		// by the time CreateWindowExW returns, WM_NCCREATE has already been processed, so it's always safe to delete the memory here
		return get_window();
	}

	static LRESULT CALLBACK message_proc_helper(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)
		{
		case WM_NCCREATE: //sadly this is not actually the first message the window receives, just the first one that's useful.  user32, sucking it hardcore, as ever.
			{
				CREATESTRUCTW* cs(reinterpret_cast<CREATESTRUCTW*>(lParam));
				std::pair<void*, void*>* ptrs(reinterpret_cast<std::pair<void*, void*>*>(cs->lpCreateParams));
				::SetWindowLongPtrW(wnd, 0, reinterpret_cast<LONG_PTR>(ptrs->first));
				cs->lpCreateParams = ptrs->second;
			}
		default:
			{
				window* w(reinterpret_cast<window*>(::GetWindowLongPtrW(wnd, 0)));
				if(w)
				{
					return w->filtering_message_proc(wnd, message, wParam, lParam);
				}
				else
				{
					return ::DefWindowProcW(wnd, message, wParam, lParam);
				}
			}
		}
	}

	int pump_messages()
	{
		MSG msg = {0};
		while(BOOL rv = ::GetMessageW(&msg, NULL, 0, 0))
		{
			if(rv == -1)
			{
				return -1;
			}
			if(NULL == accelerator_table || !::TranslateAcceleratorW(msg.hwnd, accelerator_table, &msg))
			{
				::TranslateMessage(&msg);
				::DispatchMessageW(&msg);
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

	void invalidate()
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

	void set_on_top(bool ontop)
	{
		::SetWindowPos(get_window(), ontop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
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

	SIZE get_window_size() const
	{
		RECT window_dimensions(get_window_rect());
		SIZE sz = { window_dimensions.right - window_dimensions.left, window_dimensions.bottom - window_dimensions.top };
		return sz;
	}

	BOOL post_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ::PostMessageW(get_window(), msg, wParam, lParam);
	}

	LRESULT send_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ::SendMessageW(get_window(), msg, wParam, lParam);
	}

	BOOL show_window(int cmd_show)
	{
		return ::ShowWindow(get_window(), cmd_show);
	}

	BOOL update_window()
	{
		return ::UpdateWindow(get_window());
	}

	BOOL set_placement(WINDOWPLACEMENT* placement)
	{
		placement->length = sizeof(WINDOWPLACEMENT);
		return ::SetWindowPlacement(get_window(), placement);
	}

	WINDOWPLACEMENT get_placement() const
	{
		WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
		::GetWindowPlacement(get_window(), &placement);
		return placement;
	}

	BOOL set_window_text(const wchar_t* str)
	{
		return ::SetWindowTextW(get_window(), str);
	}

	HRESULT set_window_theme(const wchar_t* sub_app_name, const wchar_t* sub_id_list)
	{
		return ::SetWindowTheme(get_window(), sub_app_name, sub_id_list);
	}

protected:
	HINSTANCE instance;

private:
	void register_class()
	{
		if(0 == ::RegisterClassExW(&window_class))
		{
			switch(::GetLastError())
			{
			case ERROR_CLASS_ALREADY_EXISTS:
				registered = false;
				break;
			default:
				throw std::runtime_error("Could not register window class");
			}
		}
	}

	void unregister_class()
	{
		if(registered)
		{
			::UnregisterClassW(window_class.lpszClassName, instance);
		}
	}

	window(const window&);
	HWND window_handle;
	std::set<message_handler*> message_handlers;
	HACCEL accelerator_table;

	WNDCLASSEXW window_class;
	bool registered;
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
		WNDCLASSEXW wc = {0};
		wc.cbSize = sizeof(WNDCLASSEXW);
		wc.style = style;
		wc.lpfnWndProc = &dialogue::message_proc_helper;
		wc.cbWndExtra = DLGWINDOWEXTRA + sizeof(dialogue*);
		wc.hInstance = instance;
		wc.hIcon = icon;
		wc.hCursor = cursor;
		wc.hbrBackground = background;
		wc.lpszMenuName = menu_name;
		wc.lpszClassName = class_name;
		window_class = ::RegisterClassExW(&wc);
		if(0 == window_class && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
		{
			throw std::runtime_error("Could not register window class");
		}
	}

	INT_PTR display_dialogue(const wchar_t* template_name, HWND parent)
	{
		return ::DialogBoxParamW(instance, template_name, parent, &dialogue::dummy_dialogue_proc, reinterpret_cast<LPARAM>(this));
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
				::SetWindowLongPtrW(wnd, DLGWINDOWEXTRA, reinterpret_cast<LONG_PTR>(d));
			}
		default:
			{
				dialogue* d(reinterpret_cast<dialogue*>(::GetWindowLongPtrW(wnd, DLGWINDOWEXTRA)));
				if(d)
				{
					return d->filtering_message_proc(wnd, message, wParam, lParam);
				}
				else
				{
					return ::DefDlgProcW(wnd, message, wParam, lParam);
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
		return ::PostMessageW(get_window(), msg, wParam, lParam);
	}

	LRESULT send_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ::SendMessageW(get_window(), msg, wParam, lParam);
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
