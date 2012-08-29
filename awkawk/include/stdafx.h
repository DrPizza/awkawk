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

#pragma comment(lib, "delayimp.lib")

#include <sdkddkver.h>

#define NOMINMAX
#define STRICT
#define ISOLATION_AWARE_ENABLED 1

#pragma warning(disable:4127) // conditional is constant
#pragma warning(disable:4189) // local initialized but not referenced
#pragma warning(disable:4345) // new T() default initializes
#pragma warning(disable:4995) // deprecated functions

#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS

#include <objbase.h>
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "avrt.lib")

#include <uxtheme.h>
#include <vssym32.h>
#pragma comment(lib, "uxtheme.lib")

#include <dwmapi.h>

#define DBGHELP_TRANSLATE_TCHAR
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

#include <windowsx.h>

#include <fstream>
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
#include <algorithm>
#include <functional>
#include <numeric>
#include <memory>

#include <comutil.h>
#include <comdef.h>
#include <commdlg.h>
#include <tchar.h>

#ifdef _DEBUG
// sadly, enabling d3d debug breaks AdviseNotify. WTF.
//#define D3D_DEBUG_INFO 1
#endif

// Media Foundation
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")

// D3D
#include <d3d9.h>
#include <d3d10.h>
#include <dxgi.h>
#include <d3dx9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dxguid.lib")

// DirectShow Header Files
#include <dshow.h>
#include <Dvdmedia.h>
#include <Dvdevcod.h>
#include <dxva.h>
#pragma comment(lib, "strmiids.lib")

// VMR9
#include <vmr9.h>

// EVR
#include <dxva2api.h>
#include <evr9.h>
#include <evcode.h>
#pragma comment(lib, "dxva2.lib")
#pragma comment(lib, "evr.lib")

// ATL

#define _ATL_ALL_WARNINGS
#define _AFX_SECURE_NO_WARNINGS
#define _ATL_SECURE_NO_WARNINGS
#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atltypes.h>
#include <atlctl.h>
#include <atlhost.h>

//using namespace ATL;

#include <boost/lexical_cast.hpp>
#include <boost/utility.hpp>
#include <boost/rational.hpp>
#include "utility/scopeguard.hpp"
#include "utility/debug.hpp"
#include "utility/locks.hpp"
#include "utility/iterator.hpp"
#include "utility/threads.hpp"
#include "utility/formattime.hpp"
#include "utility/debugstream.hpp"
#include "utility/utility.hpp"

#include "dispatch/dispatch.hpp"

#if defined DEBUG || defined _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define DEBUG_CLIENTBLOCK new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_CLIENTBLOCK
#endif
