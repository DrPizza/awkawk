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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define NOMINMAX
#define _WIN32_WINNT 0x0502
#define WINVER 0x0502
#define STRICT
#define ISOLATION_AWARE_ENABLED 1
#pragma warning(disable:4995)
#pragma warning(disable:4996)

#include <objbase.h>
#include <windows.h>

// dbghelp.h needs a newer specstrings.h/sal.h than actually
// ships from MS.  So it won't compile unless I use this #define
// which is pretty fucking shoddy.
#define __out_xcount(x)
#include <dbghelp.h>

#include <windowsx.h>

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

#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <map>
#include <vector>
#include <sstream>
#include <locale>
#include <limits>
#include <cmath>

#include <comutil.h>
#include <comdef.h>
#include <commdlg.h>
#include <tchar.h>

// DirectShow Header Files
#include <dshow.h>
#include <streams.h>
#include <Dvdmedia.h>
#include <Dvdevcod.h>

// D3D, VMR9
#include <d3d9.h>
#include <d3dx9.h>
#include <vmr9.h>
#include "vmrutil.h"


#if defined DEBUG || defined _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define DEBUG_CLIENTBLOCK   new( _CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_CLIENTBLOCK
#endif

// ATL

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atltypes.h>
#include <atlctl.h>
#include <atlhost.h>

using namespace ATL;

// TODO: reference additional headers your program requires here

#include <boost/scoped_array.hpp>
#include <boost/ptr_container/ptr_container.hpp>
#include "loki/ScopeGuard.h"
#include "loki/ScopeGuardExt.h"
#include "utility/threads.hpp"
#include "utility/formattime.hpp"
#include "utility/debugstream.hpp"
