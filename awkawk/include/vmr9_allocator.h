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

#ifndef ALLOCATOR__H
#define ALLOCATOR__H

#include "stdafx.h"
#include "resource.h"
#include "util.h"

#include "utility/interlocked_containers.hpp"

struct awkawk;
struct d3d_renderer;
struct shared_texture_queue;
struct direct3d9;

_COM_SMARTPTR_TYPEDEF(IVMRSurfaceAllocatorNotify9, __uuidof(IVMRSurfaceAllocatorNotify9));
_COM_SMARTPTR_TYPEDEF(IDirect3DDeviceManager9, __uuidof(IDirect3DDeviceManager9));

//////////////////////////////////
// These are QIed and may at some point be worth implementing
// IVMRMonitorConfig9
// IVMRSurfaceAllocatorEx9
struct vmr9_allocator_presenter : IVMRSurfaceAllocator9, IVMRImagePresenter9, IVMRImagePresenterConfig9, direct3d_manager, boost::noncopyable
{
	vmr9_allocator_presenter(awkawk* player_, shared_texture_queue* texture_queue_, d3d_renderer* renderer_, HWND window_);
	virtual ~vmr9_allocator_presenter();

	// IVMRSurfaceAllocator9
	virtual STDMETHODIMP InitializeDevice(DWORD_PTR user_id, VMR9AllocationInfo* allocationInfo, DWORD* numBuffers);
	virtual STDMETHODIMP TerminateDevice(DWORD_PTR user_id);
	virtual STDMETHODIMP GetSurface(DWORD_PTR user_id, DWORD surfaceIndex, DWORD surfaceFlags, IDirect3DSurface9** surfaces);
	virtual STDMETHODIMP AdviseNotify(IVMRSurfaceAllocatorNotify9* surfaceAllocNotify);

	// IVMRImagePresenter9
	virtual STDMETHODIMP StartPresenting(DWORD_PTR user_id);
	virtual STDMETHODIMP StopPresenting(DWORD_PTR user_id);
	virtual STDMETHODIMP PresentImage(DWORD_PTR user_id, VMR9PresentationInfo* presentationInfo);

	// IVMRImagePresenterConfig9
	virtual STDMETHODIMP SetRenderingPrefs(DWORD render_flags_)
	{
		render_flags = render_flags_;
		return S_OK;
	}
	virtual STDMETHODIMP GetRenderingPrefs(DWORD *render_flags_)
	{
		if(render_flags_ == nullptr)
		{
			return E_POINTER;
		}
		*render_flags_ = render_flags;
		return S_OK;
	}

	// IUnknown
	virtual STDMETHODIMP QueryInterface(const IID& riid, void** ppvObject);
	virtual STDMETHODIMP_(ULONG) AddRef();
	virtual STDMETHODIMP_(ULONG) Release();

protected:
	// create D3DPOOL_MANAGED resources
	virtual HRESULT do_on_device_created(IDirect3DDevice9Ptr new_device);

	// create D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_reset();

	// destroy D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_lost();

	// destroy D3DPOOL_MANAGED resources
	virtual void do_on_device_destroyed()
	{
		on_device_lost();
	}

private:
	IDirect3DSurface9* allocate_single_surface(VMR9AllocationInfo* allocation_info);
	void allocate_surfaces(VMR9AllocationInfo* allocation_info, DWORD* buffer_count);

	long ref_count;

	mutable utility::critical_section cs;

	IVMRSurfaceAllocatorNotify9Ptr surface_allocator_notify;
	std::vector<ATL::CAdapt<IDirect3DSurface9Ptr> > vmr9_surfaces;

	IDirect3DSurface9Ptr staging_surface;

	awkawk* player;
	shared_texture_queue* texture_queue;
	d3d_renderer* renderer;
	HWND window;

	DWORD render_flags;

	// direct3d gubbins
	std::unique_ptr<direct3d9> d3d9;
	UINT reset_token;
	IDirect3DDeviceManager9Ptr device_manager;

	IDirect3DDevice9ExPtr device;

	// D3D methods
	void create_d3d();
	void destroy_d3d();
	void create_device();
	void destroy_device();
	void reset_device();
	bool needs_display_change() const;
	void reset();

	D3DPRESENT_PARAMETERS presentation_parameters;
	void set_device_state();

};

#endif
