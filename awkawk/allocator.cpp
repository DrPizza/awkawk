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

#include "awkawk.h"

allocator_presenter::allocator_presenter(awkawk* player_, IDirect3DDevice9Ptr device_) : ref_count(0),
                                                                                         player(player_),
                                                                                         device(device_)
{
}

allocator_presenter::~allocator_presenter()
{
}

void allocator_presenter::begin_device_loss()
{
	LOCK(cs);
	device = NULL;
	surfaces.clear();
	video_textures.clear();
	texture_locks.clear();
}

void allocator_presenter::end_device_loss(IDirect3DDevice9Ptr device_)
{
	LOCK(cs);
	device = device_;
	FAIL_THROW(surface_allocator_notify->ChangeD3DDevice(device, ::MonitorFromWindow(player->get_ui()->get_window(), MONITOR_DEFAULTTOPRIMARY)));
}

//IVMRSurfaceAllocator9
STDMETHODIMP allocator_presenter::InitializeDevice(DWORD_PTR id, VMR9AllocationInfo* allocation_info, DWORD* buffer_count)
{
	if(buffer_count == NULL)
	{
		return E_POINTER;
	}

	if(surface_allocator_notify == NULL)
	{
		return E_FAIL;
	}

	try
	{
		LOCK(cs);
		if(allocation_info->dwFlags & VMR9AllocFlag_3DRenderTarget)
		{
			allocation_info->dwFlags |= VMR9AllocFlag_TextureSurface;
		}

		std::vector<IDirect3DSurface9*> raw_surfaces;
		raw_surfaces.resize(*buffer_count);

		FAIL_THROW(surface_allocator_notify->AllocateSurfaceHelper(allocation_info, buffer_count, &raw_surfaces[0]));
		texture_locks[id].reset(new utility::critical_section());
		boost::shared_ptr<utility::critical_section> stream_cs(get_cs(id));
		LOCK(*stream_cs);
		surfaces[id].clear();
		surfaces[id].resize(raw_surfaces.size());
		for(size_t i(0); i < raw_surfaces.size(); ++i)
		{
			static_cast<IDirect3DSurface9Ptr&>(surfaces[id][i]).Attach(raw_surfaces[i], false);
		}
		IDirect3DTexture9Ptr txtr;
		FAIL_THROW(device->CreateTexture(allocation_info->dwWidth, allocation_info->dwHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &txtr, NULL));
		video_textures[id] = txtr;
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
		return ce.Error();
	}
	return S_OK;
}

STDMETHODIMP allocator_presenter::TerminateDevice(DWORD_PTR id)
{
	LOCK(cs);
	if(surfaces.find(id) == surfaces.end())
	{
		return E_FAIL;
	}
	surfaces.erase(id);
	video_textures.erase(id);
	texture_locks.erase(id);
	return S_OK;
}

STDMETHODIMP allocator_presenter::GetSurface(DWORD_PTR id, DWORD surface_index, DWORD, IDirect3DSurface9** surface)
{
	if(surface == NULL)
	{
		return E_POINTER;
	}

	LOCK(cs);
	if(surfaces.find(id) == surfaces.end())
	{
		return E_FAIL;
	}
	if(surface_index >= surfaces[id].size())
	{
		return E_FAIL;
	}

	return static_cast<IDirect3DSurface9Ptr&>(surfaces[id][surface_index]).QueryInterface(__uuidof(IDirect3DSurface9), surface);
}

STDMETHODIMP allocator_presenter::AdviseNotify(IVMRSurfaceAllocatorNotify9* surface_allocator_notify_)
{
	LOCK(cs);
	surface_allocator_notify = surface_allocator_notify_;
	return S_OK;
}

STDMETHODIMP allocator_presenter::StartPresenting(DWORD_PTR id)
{
	LOCK(cs);
	if(device == NULL)
	{
		return E_FAIL;
	}
	if(surfaces.find(id) == surfaces.end())
	{
		return E_FAIL;
	}
	return S_OK;
}

STDMETHODIMP allocator_presenter::StopPresenting(DWORD_PTR id)
{
	LOCK(cs);
	if(surfaces.find(id) == surfaces.end())
	{
		return E_FAIL;
	}
	return S_OK;
}

STDMETHODIMP allocator_presenter::PresentImage(DWORD_PTR id, VMR9PresentationInfo* presentation_info)
{
	if(presentation_info == NULL)
	{
		return E_POINTER;
	}
	if(presentation_info->lpSurf == NULL)
	{
		return E_POINTER;
	}
	LOCK(cs);
	if(surfaces.find(id) == surfaces.end())
	{
		return E_FAIL;
	}

	try
	{
		SIZE video_dimensions(player->get_video_dimensions());
		SIZE new_video_dimensions(video_dimensions);
		new_video_dimensions.cx = (new_video_dimensions.cy * presentation_info->szAspectRatio.cx) / presentation_info->szAspectRatio.cy;
		if(video_dimensions.cx != new_video_dimensions.cx
		|| video_dimensions.cy != new_video_dimensions.cy)
		{
			player->set_video_dimensions(new_video_dimensions);
		}

		IDirect3DSurface9Ptr surf;
		FAIL_THROW(static_cast<IDirect3DTexture9Ptr&>(video_textures[id])->GetSurfaceLevel(0, &surf));
		boost::shared_ptr<utility::critical_section> stream_cs(get_cs(id));
		LOCK(*stream_cs);
		FAIL_THROW(device->StretchRect(presentation_info->lpSurf, NULL, surf, NULL, D3DTEXF_NONE));
		player->signal_new_frame();
		return S_OK;
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
		return ce.Error();
	}
}

STDMETHODIMP allocator_presenter::QueryInterface(const IID& iid, void** target)
{
	if(target == NULL)
	{
		return E_POINTER;
	}
	else if(iid == IID_IVMRSurfaceAllocator9)
	{
		*target = static_cast<IVMRSurfaceAllocator9*>(this);
		AddRef();
		return S_OK;
	}
	else if(iid == IID_IVMRImagePresenter9)
	{
		*target = static_cast<IVMRImagePresenter9*>(this);
		AddRef();
		return S_OK;
	}
	else if(iid == IID_IUnknown)
	{
		*target = static_cast<IUnknown*>(static_cast<IVMRSurfaceAllocator9*>(this));
		AddRef();
		return S_OK;
	}
	else
	{
		*target = NULL;
		//wdout << L"unknown interface " << riid << std::endl;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) allocator_presenter::AddRef()
{
	return ::InterlockedIncrement(&ref_count);
}

STDMETHODIMP_(ULONG) allocator_presenter::Release()
{
	ULONG ret(::InterlockedDecrement(&ref_count));
	if(ret == 0)
	{
		delete this;
	}
	return ret;
}
