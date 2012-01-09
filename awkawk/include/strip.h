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

#ifndef STRIP__H
#define STRIP__H

#include "stdafx.h"
#include "util.h"

// a textured strip
struct strip : direct3d_object
{
	struct vertex
	{
		enum { format = (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1) };
		struct position3
		{
			position3() : x(0.0f), y(0.0f), z(0.0f)
			{}
			position3(float x_, float y_, float z_) : x(x_), y(y_), z(z_)
			{}
			float x, y, z;
		};

		position3 position; // The position
		D3DCOLOR diffuse;
		FLOAT tu, tv; // The texture coordinates
	};

	strip(size_t size, direct3d_manager* manager_) : direct3d_object(manager_)
	{
		vertices.resize(size);
	}

	~strip() {
	}

	void copy_to_buffer()
	{
		void* buffer;
		FAIL_THROW(vertex_buffer->Lock(0, 0, &buffer, 0));
		std::memcpy(buffer, &vertices[0], sizeof(vertex) * vertices.size());
		FAIL_THROW(vertex_buffer->Unlock());
	}
	void draw(IDirect3DTexture9Ptr texture)
	{
		FAIL_THROW(device->SetTexture(0, texture));
		FAIL_THROW(device->SetStreamSource(0, vertex_buffer, 0, sizeof(strip::vertex)));
		FAIL_THROW(device->SetFVF(strip::vertex::format));
		FAIL_THROW(device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, static_cast<UINT>(vertices.size() - 2)));
		FAIL_THROW(device->SetStreamSource(0, nullptr, 0, 0));
		FAIL_THROW(device->SetTexture(0, nullptr));
	}
	std::vector<vertex> vertices;

protected:
	// create D3DPOOL_MANAGED resources
	virtual HRESULT do_on_device_created(IDirect3DDevice9Ptr new_device)
	{
		device = new_device;
		return S_OK;
	}
	// create D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_reset()
	{
		FAIL_RET(device->CreateVertexBuffer(static_cast<UINT>(sizeof(vertex) * vertices.size()), D3DUSAGE_WRITEONLY, strip::vertex::format, D3DPOOL_DEFAULT, &vertex_buffer, nullptr));
		return S_OK;
	}
	// destroy D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_lost()
	{
		vertex_buffer = nullptr;
		return S_OK;
	}
	// destroy D3DPOOL_MANAGED resources
	virtual void do_on_device_destroyed()
	{
		device = nullptr;
	}

private:
	IDirect3DVertexBuffer9Ptr vertex_buffer;
	IDirect3DDevice9Ptr device;
};

#endif
