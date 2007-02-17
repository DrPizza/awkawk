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
#define NTDDI_VERSION NTDDI_LONGHORN
#define STRICT
#define ISOLATION_AWARE_ENABLED 1
#pragma warning(disable:4995)
#pragma warning(disable:4996)

#include <objbase.h>
#include <windows.h>
#include <uxtheme.h>

// dbghelp.h needs a newer specstrings.h/sal.h than actually
// ships from MS.  So it won't compile unless I use this #define
// which is pretty fucking shoddy.
#ifndef __out_xcount
#define __out_xcount(size)                            __allowed(on_parameter)
#endif
#include <dbghelp.h>

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
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/ptr_container/ptr_container.hpp>
#include "loki/ScopeGuard.h"
#include "loki/ScopeGuardExt.h"
#include "utility/threads.hpp"
#include "utility/formattime.hpp"
#include "utility/debugstream.hpp"
