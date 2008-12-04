//  Copyright (C) 2008 Peter Bright
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

#include "direct3d.h"

void direct3d9::create_d3d()
{
#define USE_D3D9EX
#ifdef USE_D3D9EX
	HMODULE d3d9_dll(::GetModuleHandleW(L"d3d9.dll"));
	typedef HRESULT (WINAPI *d3dcreate9ex_proc)(UINT, IDirect3D9Ex**);
	d3dcreate9ex_proc d3dcreate9ex(reinterpret_cast<d3dcreate9ex_proc>(::GetProcAddress(d3d9_dll, "Direct3DCreate9Ex")));
	if(d3dcreate9ex != NULL)
	{
		if(S_OK == d3dcreate9ex(D3D_SDK_VERSION, &d3d9ex))
		{
			d3d9 = d3d9ex;
		}
		else
		{
			dout << "Attempt to use D3D9Ex failed, falling back to D3D9" << std::endl;
		}
	}
	if(d3d9 == NULL)
#endif
	{
		IDirect3D9* raw(::Direct3DCreate9(D3D_SDK_VERSION));
		if(raw)
		{
			d3d9.Attach(raw);
		}
	}
	if(d3d9 == NULL)
	{
		_com_raise_error(E_FAIL);
	}
}
