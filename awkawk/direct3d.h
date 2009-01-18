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

#pragma once

#ifndef DIRECT3D__H
#define DIRECT3D__H

#include <d3d9.h>
#include <d3dx9.h>

#include "util.h"

struct direct3d9
{
	direct3d9()
	{
		create_d3d();
	}

	~direct3d9()
	{
		destroy_d3d();
	}

	IDirect3D9Ptr get_d3d9() const
	{
		return d3d9;
	}

	IDirect3D9ExPtr get_d3d9ex() const
	{
		return d3d9ex;
	}

	bool is_d3d9ex() const
	{
		return d3d9ex != NULL;
	}

private:
	IDirect3D9Ptr d3d9;
	IDirect3D9ExPtr d3d9ex;

	void create_d3d();

	void destroy_d3d()
	{
		d3d9 = NULL;
		d3d9ex = NULL;
	}
};

#endif
