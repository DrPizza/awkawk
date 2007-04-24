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

#ifndef ALLOCATOR__H
#define ALLOCATOR__H

#include "stdafx.h"
#include "resource.h"
#include "util.h"

_COM_SMARTPTR_TYPEDEF(IVMRSurfaceAllocatorNotify9, __uuidof(IVMRSurfaceAllocatorNotify9));

//////////////////////////////////
// These are QIed and may at some point be worth implementing
// IVMRMonitorConfig9
// IVMRSurfaceAllocatorEx9
struct surface_allocator : IVMRSurfaceAllocator9, IVMRImagePresenter9, IVMRImagePresenterConfig9, device_loss_handler
{
	surface_allocator(awkawk* player_, IDirect3DDevice9Ptr device_);
	virtual ~surface_allocator();

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
		if(render_flags_ == NULL)
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

	void begin_device_loss();
	void end_device_loss(IDirect3DDevice9Ptr device);

	IDirect3DTexture9Ptr get_video_texture(DWORD_PTR id)
	{
		LOCK(cs);
		return static_cast<IDirect3DTexture9Ptr&>(video_textures[id]);
	}

	boost::shared_ptr<utility::critical_section> get_cs(DWORD_PTR id)
	{
		LOCK(cs);
		return texture_locks[id];
	}

	bool rendering(DWORD_PTR id) const
	{
		LOCK(cs);
		return texture_locks.find(id) != texture_locks.end();
	}
private:
	surface_allocator(const surface_allocator&);

	long ref_count;

	mutable utility::critical_section cs;

	IDirect3DDevice9Ptr device;
	IVMRSurfaceAllocatorNotify9Ptr surface_allocator_notify;
	std::map<DWORD_PTR, std::vector<CAdapt<IDirect3DSurface9Ptr> > > surfaces;
	std::map<DWORD_PTR, CAdapt<IDirect3DTexture9Ptr> > video_textures;
	std::map<DWORD_PTR, boost::shared_ptr<utility::critical_section> > texture_locks;

	awkawk* player;
	DWORD render_flags;
};

#endif
