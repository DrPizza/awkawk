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

#include "player.h"

surface_allocator::surface_allocator(Player* player_, HWND window_, IDirect3DDevice9Ptr device_) : ref_count(0), player(player_), window(window_), device(device_)
{
}

surface_allocator::~surface_allocator()
{
}

void surface_allocator::begin_device_loss()
{
	device = NULL;
	surfaces.clear();
	video_textures.clear();
}

void surface_allocator::end_device_loss(IDirect3DDevice9Ptr device_)
{
	device = device_;
	FAIL_THROW(surface_allocator_notify->ChangeD3DDevice(device, ::MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY)));
}

//IVMRSurfaceAllocator9
STDMETHODIMP surface_allocator::InitializeDevice(DWORD_PTR id, VMR9AllocationInfo* allocInfo, DWORD* numBuffers)
{
	critical_section::lock l(cs);
	if(numBuffers == NULL)
	{
		return E_POINTER;
	}

	if(surface_allocator_notify == NULL)
	{
		return E_FAIL;
	}

	try
	{
		if(allocInfo->dwFlags & VMR9AllocFlag_3DRenderTarget)
		{
			allocInfo->dwFlags |= VMR9AllocFlag_TextureSurface;
		}

		std::vector<IDirect3DSurface9*> rawSurfaces;
		rawSurfaces.resize(*numBuffers);

		FAIL_THROW(surface_allocator_notify->AllocateSurfaceHelper(allocInfo, numBuffers, &rawSurfaces[0]));
		surfaces[id].clear();
		surfaces[id].resize(rawSurfaces.size());
		for(size_t i(0); i < rawSurfaces.size(); ++i)
		{
			static_cast<IDirect3DSurface9Ptr&>(surfaces[id][i]).Attach(rawSurfaces[i], false);
		}
		IDirect3DTexture9Ptr txtr;
		FAIL_THROW(device->CreateTexture(allocInfo->dwWidth, allocInfo->dwHeight, 1, D3DUSAGE_DYNAMIC, allocInfo->Format, D3DPOOL_DEFAULT, &txtr, NULL));
		video_textures[id] = txtr;
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
		return ce.Error();
	}
	return S_OK;
}

STDMETHODIMP surface_allocator::TerminateDevice(DWORD_PTR id)
{
	critical_section::lock l(cs);
	surfaces[id].clear();
	video_textures[id] = NULL;
	return S_OK;
}

STDMETHODIMP surface_allocator::GetSurface(DWORD_PTR id, DWORD surfaceIndex, DWORD surfaceFlags, IDirect3DSurface9** lplpSurface)
{
	critical_section::lock l(cs);
	if(lplpSurface == NULL)
	{
		return E_POINTER;
	}

	if(surfaceIndex >= surfaces[id].size())
	{
		return E_FAIL;
	}

	return static_cast<IDirect3DSurface9Ptr&>(surfaces[id][surfaceIndex]).QueryInterface(__uuidof(IDirect3DSurface9), lplpSurface);
}

STDMETHODIMP surface_allocator::AdviseNotify(IVMRSurfaceAllocatorNotify9* lpIVMRSurfAllocNotify)
{
	critical_section::lock l(cs);
	surface_allocator_notify = lpIVMRSurfAllocNotify;
	return S_OK;
}

STDMETHODIMP surface_allocator::StartPresenting(DWORD_PTR)
{
	critical_section::lock l(cs);
	if(device == NULL)
	{
		return E_FAIL;
	}
	return S_OK;
}

STDMETHODIMP surface_allocator::StopPresenting(DWORD_PTR)
{
	return S_OK;
}

STDMETHODIMP surface_allocator::PresentImage(DWORD_PTR id, VMR9PresentationInfo* presentationInfo)
{
	if(presentationInfo == NULL)
	{
		return E_POINTER;
	}
	if(presentationInfo->lpSurf == NULL)
	{
		return E_POINTER;
	}

	try
	{
		critical_section::lock l(cs);
		player->set_video_dimensions(presentationInfo->szAspectRatio);
		IDirect3DTexture9Ptr txtr;
		presentationInfo->lpSurf->GetContainer(IID_IDirect3DTexture9, reinterpret_cast<void**>(&txtr));
		video_textures[id] = txtr;
		player->render();

		return S_OK;
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
		return ce.Error();
	}
}

STDMETHODIMP surface_allocator::QueryInterface(const IID& riid, void** ppvObject)
{
	if(ppvObject == NULL)
	{
		return E_POINTER;
	}
	else if(riid == IID_IVMRSurfaceAllocator9)
	{
		*ppvObject = static_cast<IVMRSurfaceAllocator9*>(this);
		AddRef();
		return S_OK;
	}
	else if(riid == IID_IVMRImagePresenter9)
	{
		*ppvObject = static_cast<IVMRImagePresenter9*>(this);
		AddRef();
		return S_OK;
	}
	else if(riid == IID_IUnknown)
	{
		*ppvObject = static_cast<IUnknown*>(static_cast<IVMRSurfaceAllocator9*>(this));
		AddRef();
		return S_OK;
	}
	//else
	//{
	//	wdout << L"unknown interface " << riid << std::endl;
	//}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) surface_allocator::AddRef()
{
	return ::InterlockedIncrement(&ref_count);
}

STDMETHODIMP_(ULONG) surface_allocator::Release()
{
	ULONG ret(::InterlockedDecrement(&ref_count));
	if(ret == 0)
	{
		delete this;
	}
	return ret;
}
