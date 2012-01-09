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

#include "vmr9_allocator.h"
#include "awkawk.h"
#include "d3d_renderer.h"
#include "shared_texture_queue.h"
#include "direct3d.h"

vmr9_allocator_presenter::vmr9_allocator_presenter(awkawk* player_,
                                                   shared_texture_queue* texture_queue_,
                                                   d3d_renderer* renderer_,
                                                   HWND window_) : direct3d_manager(nullptr), ref_count(0),
                                                                   cs("allocator_presenter"),
                                                                   player(player_),
                                                                   texture_queue(texture_queue_),
                                                                   renderer(renderer_),
                                                                   window(window_) {
	create_d3d();
	create_device();
}

vmr9_allocator_presenter::~vmr9_allocator_presenter()
{
	destroy_device();
	destroy_d3d();
}

HRESULT vmr9_allocator_presenter::do_on_device_created(IDirect3DDevice9Ptr new_device)
{
	device = new_device;
	if(surface_allocator_notify)
	{
		FAIL_THROW(surface_allocator_notify->ChangeD3DDevice(device, ::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST)));
	}
	texture_queue->set_producer(new_device);
	return S_OK;
}

HRESULT vmr9_allocator_presenter::do_on_device_reset()
{
	if(surface_allocator_notify)
	{
		FAIL_THROW(surface_allocator_notify->ChangeD3DDevice(device, ::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST)));
	}
	return S_OK;
}

HRESULT vmr9_allocator_presenter::do_on_device_lost()
{
	device = nullptr;
	texture_queue->set_producer(nullptr);
	vmr9_surfaces.clear();
	return S_OK;
}

IDirect3DSurface9* vmr9_allocator_presenter::allocate_single_surface(VMR9AllocationInfo* allocation_info) {
	if(allocation_info->Format == D3DFMT_UNKNOWN) {
		D3DDISPLAYMODE dm; 
		FAIL_THROW(device->GetDisplayMode(NULL, &dm));
		allocation_info->Format = dm.Format;
	}
	
	IDirect3DSurface9* surf(nullptr);
	if(allocation_info->dwFlags & VMR9AllocFlag_TextureSurface) {
		IDirect3DTexture9Ptr tex;
		device->CreateTexture(allocation_info->dwWidth, allocation_info->dwHeight, 1, D3DUSAGE_RENDERTARGET, allocation_info->Format, allocation_info->Pool, &tex, nullptr);
		if(tex) {
			tex->GetSurfaceLevel(0, &surf);
		}
	} else if(allocation_info->dwFlags & VMR9AllocFlag_OffscreenSurface) {
		device->CreateOffscreenPlainSurface(allocation_info->dwWidth, allocation_info->dwHeight, allocation_info->Format, allocation_info->Pool, &surf, nullptr);
	} else if(allocation_info->dwFlags & VMR9AllocFlag_3DRenderTarget) {
		device->CreateRenderTarget(allocation_info->dwWidth, allocation_info->dwHeight, allocation_info->Format, D3DMULTISAMPLE_NONE, 0, TRUE, &surf, nullptr);
	}
	return surf;
}

void vmr9_allocator_presenter::allocate_surfaces(VMR9AllocationInfo* allocation_info, DWORD* buffer_count) {
	std::vector<IDirect3DSurface9*> raw_surfaces(*buffer_count);
#define USE_ALLOCATE_SURFACE_HELPER 1
#if USE_ALLOCATE_SURFACE_HELPER
	FAIL_THROW(surface_allocator_notify->AllocateSurfaceHelper(allocation_info, buffer_count, &raw_surfaces[0]));
#else
	for(DWORD i(0); i < *buffer_count; ++i) {
		raw_surfaces[i] = allocate_single_surface(allocation_info);
	}
#endif

	LOCK(cs);
	vmr9_surfaces.clear();
	vmr9_surfaces.resize(raw_surfaces.size());
	for(size_t i(0); i < raw_surfaces.size(); ++i)
	{
		static_cast<IDirect3DSurface9Ptr&>(vmr9_surfaces[i]).Attach(raw_surfaces[i], false);
	}
}

//IVMRSurfaceAllocator9
STDMETHODIMP vmr9_allocator_presenter::InitializeDevice(DWORD_PTR, VMR9AllocationInfo* allocation_info, DWORD* buffer_count)
{
	if(buffer_count == nullptr)
	{
		return E_POINTER;
	}

	if(*buffer_count == 0)
	{
		return E_INVALIDARG;
	}

	if(surface_allocator_notify == nullptr)
	{
		return E_FAIL;
	}

	try
	{
#if 1
		dout << "requesting " << *buffer_count << " buffers" << std::endl
		     << "\tsize: " << allocation_info->dwWidth << " x " << allocation_info->dwHeight << std::endl
		     << "\tminumum buffers: " << allocation_info->MinBuffers << std::endl
		     << "\trequesting format                         " << format_name(allocation_info->Format) << std::endl
		     << "\trequesting pool                           " << pool_name(allocation_info->Pool) << std::endl
		     << "\trequesting VMR9AllocFlag_3DRenderTarget   " << !!(VMR9AllocFlag_3DRenderTarget & allocation_info->dwFlags) << std::endl
		     << "\trequesting VMR9AllocFlag_DXVATarget       " << !!(VMR9AllocFlag_DXVATarget & allocation_info->dwFlags) << std::endl
		     << "\trequesting VMR9AllocFlag_TextureSurface   " << !!(VMR9AllocFlag_TextureSurface & allocation_info->dwFlags) << std::endl
		     << "\trequesting VMR9AllocFlag_OffscreenSurface " << !!(VMR9AllocFlag_OffscreenSurface & allocation_info->dwFlags) << std::endl
		     << "\trequesting VMR9AllocFlag_RGBDynamicSwitch " << !!(VMR9AllocFlag_RGBDynamicSwitch & allocation_info->dwFlags) << std::endl;
#endif

		allocate_surfaces(allocation_info, buffer_count);

#if 1
		D3DSURFACE_DESC desc = { D3DFMT_UNKNOWN };
		vmr9_surfaces[0]->GetDesc(&desc);

		dout << "received:" << std::endl;
		print_surface_desc(desc);

		IDirect3DTexture9* tex(nullptr);
		vmr9_surfaces[0]->GetContainer(IID_IDirect3DTexture9, reinterpret_cast<void**>(&tex));
		if(tex) {
			dout << "\tsurface is in a texture" << std::endl;
			tex->Release();
		} else {
			dout << "\tsurface is standalone" << std::endl;
		}
#endif

		D3DDISPLAYMODE dm; 
		FAIL_THROW(device->GetDisplayMode(NULL, &dm));
		D3DFORMAT format(allocation_info->Format > '0000' ? dm.Format : allocation_info->Format);

		shared_texture_queue::shared_texture_queue_desc texture_desc = {
			format,
			allocation_info->dwWidth,
			allocation_info->dwHeight,
			*buffer_count
		};
		
		texture_queue->set_parameters(texture_desc);

		FAIL_THROW(device->CreateRenderTarget(allocation_info->dwWidth, allocation_info->dwHeight, format, D3DMULTISAMPLE_NONE, 0, TRUE, &staging_surface, nullptr));
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
		return ce.Error();
	}
	return S_OK;
}

STDMETHODIMP vmr9_allocator_presenter::TerminateDevice(DWORD_PTR)
{
	LOCK(cs);
	vmr9_surfaces.clear();
	return S_OK;
}

STDMETHODIMP vmr9_allocator_presenter::GetSurface(DWORD_PTR, DWORD surface_index, DWORD, IDirect3DSurface9** surface)
{
	if(surface == nullptr)
	{
		return E_POINTER;
	}

	LOCK(cs);
	if(surface_index >= vmr9_surfaces.size())
	{
		return E_FAIL;
	}

	return static_cast<IDirect3DSurface9Ptr&>(vmr9_surfaces[surface_index]).QueryInterface(__uuidof(IDirect3DSurface9), surface);
}

STDMETHODIMP vmr9_allocator_presenter::AdviseNotify(IVMRSurfaceAllocatorNotify9* surface_allocator_notify_)
{
	LOCK(cs);
	surface_allocator_notify = surface_allocator_notify_;
	return surface_allocator_notify->SetD3DDevice(device, ::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST));
}

STDMETHODIMP vmr9_allocator_presenter::StartPresenting(DWORD_PTR)
{
	if(device == nullptr)
	{
		return E_FAIL;
	}
	return S_OK;
}

STDMETHODIMP vmr9_allocator_presenter::StopPresenting(DWORD_PTR)
{
	return S_OK;
}

STDMETHODIMP vmr9_allocator_presenter::PresentImage(DWORD_PTR, VMR9PresentationInfo* presentation_info)
{
	if(surface_allocator_notify == nullptr)
	{
		return E_UNEXPECTED;
	}
	if(presentation_info == nullptr)
	{
		return E_POINTER;
	}
	if(presentation_info->lpSurf == nullptr)
	{
		return E_POINTER;
	}

	try
	{
		try
		{
			//if(needs_display_change())
			//{
			//	reset_device();
			//}
			reset();
		}
		catch(_com_error& ce)
		{
			derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
		}

		SIZE video_dimensions(player->get_video_dimensions());
		SIZE new_video_dimensions(video_dimensions);
		new_video_dimensions.cx = (new_video_dimensions.cy * presentation_info->szAspectRatio.cx) / presentation_info->szAspectRatio.cy;
		if(video_dimensions.cx != new_video_dimensions.cx
		|| video_dimensions.cy != new_video_dimensions.cy)
		{
			player->set_video_dimensions(new_video_dimensions);
		}

#if 0
		dout << "Frame received: " << std::endl
		     << "\tVMR9Sample_SyncPoint    : " << !!(presentation_info->dwFlags & VMR9Sample_SyncPoint) << std::endl
		     << "\tVMR9Sample_Preroll      : " << !!(presentation_info->dwFlags & VMR9Sample_Preroll) << std::endl
		     << "\tVMR9Sample_Discontinuity: " << !!(presentation_info->dwFlags & VMR9Sample_Discontinuity) << std::endl
		     << "\tVMR9Sample_TimeValid    : " << !!(presentation_info->dwFlags & VMR9Sample_TimeValid) << std::endl
		     << "\tstart: " << presentation_info->rtStart << std::endl
		     << "\tend  : " << presentation_info->rtEnd << std::endl;
#endif

		// LockRect has the effect of flushing outstanding writes to GPU memory. If I don't do the lock/unlock pair, I get tearing.

		FAIL_THROW(device->StretchRect(presentation_info->lpSurf, nullptr, staging_surface, nullptr, D3DTEXF_NONE));
		D3DLOCKED_RECT lr;
		FAIL_THROW(staging_surface->LockRect(&lr, nullptr, D3DLOCK_READONLY));
		FAIL_THROW(staging_surface->UnlockRect());

		IDirect3DTexture9Ptr destination_texture(texture_queue->producer_dequeue());
		IDirect3DSurface9Ptr surf;
		FAIL_THROW(destination_texture->GetSurfaceLevel(0, &surf));
		FAIL_THROW(device->StretchRect(staging_surface, nullptr, surf, nullptr, D3DTEXF_NONE));
		FAIL_THROW(surf->LockRect(&lr, nullptr, D3DLOCK_READONLY));
		FAIL_THROW(surf->UnlockRect());

		texture_queue->producer_enqueue(destination_texture);

		//static const GUID timestamp_guid      = { 0x6af09a08, 0x5e5b, 0x40e7, 0x99, 0x0c, 0xa4, 0x21, 0x12, 0x4b, 0x97, 0xa6 };
		//textures.first ->SetPrivateData(timestamp_guid, &presentation_info->rtStart, sizeof(&presentation_info->rtStart), 0);
		//textures.second->SetPrivateData(timestamp_guid, &presentation_info->rtStart, sizeof(&presentation_info->rtStart), 0);

		//renderer->publish_texture(textures);
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
		return ce.Error();
	}
	renderer->signal_new_frame();
	return S_OK;
}

STDMETHODIMP vmr9_allocator_presenter::QueryInterface(const IID& iid, void** target)
{
	if(target == nullptr)
	{
		return E_POINTER;
	}
	     if(iid == IID_IVMRSurfaceAllocator9)
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
		*target = nullptr;
		//wdout << L"unknown interface " << riid << std::endl;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) vmr9_allocator_presenter::AddRef()
{
	return ::InterlockedIncrement(&ref_count);
}

STDMETHODIMP_(ULONG) vmr9_allocator_presenter::Release()
{
	ULONG ret(::InterlockedDecrement(&ref_count));
	if(ret == 0)
	{
		delete this;
	}
	return ret;
}

void vmr9_allocator_presenter::create_d3d() {
	d3d9.reset(new direct3d9());
	DXVA2CreateDirect3DDeviceManager9(&reset_token, &device_manager);
}

void vmr9_allocator_presenter::destroy_d3d() {
	d3d9.reset();
}

void vmr9_allocator_presenter::create_device()
{
#if defined(USE_RGBRAST)
	HMODULE rgb_rast(::LoadLibraryW(L"rgb9rast.dll"));
	if(rgb_rast != NULL)
	{
		FARPROC rgb_rast_register(::GetProcAddress(rgb_rast, "D3D9GetSWInfo"));
		d3d9->get_d3d9()->RegisterSoftwareDevice(reinterpret_cast<void*>(rgb_rast_register));
	}
	const D3DDEVTYPE dev_type(D3DDEVTYPE_SW);
	const DWORD vertex_processing(D3DCREATE_SOFTWARE_VERTEXPROCESSING);
#elif defined(USE_REFFAST)
	const D3DDEVTYPE dev_type(D3DDEVTYPE_REF);
	const DWORD vertex_processing(D3DCREATE_SOFTWARE_VERTEXPROCESSING);
#else
	const D3DDEVTYPE dev_type(D3DDEVTYPE_HAL);
	const DWORD vertex_processing(D3DCREATE_HARDWARE_VERTEXPROCESSING);
#endif

	UINT device_ordinal(0);
	for(UINT ord(0); ord < d3d9->get_d3d9()->GetAdapterCount(); ++ord)
	{
		if(d3d9->get_d3d9()->GetAdapterMonitor(ord) == ::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST))
		{
			device_ordinal = ord;
			break;
		}
	}

	D3DDISPLAYMODE dm;
	FAIL_THROW(d3d9->get_d3d9()->GetAdapterDisplayMode(device_ordinal, &dm));

	D3DCAPS9 caps;
	FAIL_THROW(d3d9->get_d3d9()->GetDeviceCaps(device_ordinal, dev_type, &caps));
	if((caps.TextureCaps & D3DPTEXTURECAPS_POW2) && !(caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
	{
		::MessageBoxW(window, L"The device does not support non-power of 2 textures.  awkawk cannot continue.", L"Fatal Error", MB_ICONERROR);
		throw std::runtime_error("The device does not support non-power of 2 textures.  awkawk cannot continue.");
	}
	if(caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
	{
		::MessageBoxW(window, L"The device does not support non-square textures.  awkawk cannot continue.", L"Fatal Error", MB_ICONERROR);
		throw std::runtime_error("The device does not support non-square textures.  awkawk cannot continue.");
	}

	std::memset(&presentation_parameters, 0, sizeof(D3DPRESENT_PARAMETERS));
	presentation_parameters.BackBufferWidth = std::max(static_cast<UINT>(1920), dm.Width);
	presentation_parameters.BackBufferHeight = std::max(static_cast<UINT>(1080), dm.Height);
	presentation_parameters.BackBufferFormat = dm.Format;
	presentation_parameters.BackBufferCount = 4;
#ifdef USE_MULTISAMPLING
	presentation_parameters.MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;
	DWORD qualityLevels(0);
	FAIL_THROW(d3d->CheckDeviceMultiSampleType(device_ordinal, dev_type, presentation_parameters.BackBufferFormat, presentation_parameters.Windowed, presentation_parameters.MultiSampleType, &qualityLevels));
	presentation_parameters.MultiSampleQuality = qualityLevels - 1;
#else
	presentation_parameters.MultiSampleType = D3DMULTISAMPLE_NONE;
	presentation_parameters.MultiSampleQuality = 0;
#endif
	presentation_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentation_parameters.hDeviceWindow = window;
	presentation_parameters.Windowed = TRUE;
	presentation_parameters.EnableAutoDepthStencil = TRUE;
	presentation_parameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentation_parameters.Flags = D3DPRESENTFLAG_VIDEO;
	presentation_parameters.FullScreen_RefreshRateInHz = 0;
	presentation_parameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	D3DPRESENT_PARAMETERS parameters(presentation_parameters);
	FAIL_THROW(d3d9->get_d3d9()->CreateDeviceEx(device_ordinal,
	                                            dev_type,
	                                            window,
	                                            vertex_processing | D3DCREATE_MULTITHREADED | D3DCREATE_NOWINDOWCHANGES | D3DCREATE_FPU_PRESERVE,
	                                            &parameters,
	                                            nullptr,
	                                            &device));

	device_manager->ResetDevice(device, reset_token);

	set_device_state();

	on_device_created(device);
	on_device_reset();
}

void vmr9_allocator_presenter::set_device_state()
{
	FAIL_THROW(device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
	FAIL_THROW(device->SetRenderState(D3DRS_LIGHTING, FALSE));
	FAIL_THROW(device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
	FAIL_THROW(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	FAIL_THROW(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
	FAIL_THROW(device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE));
	FAIL_THROW(device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE));

	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP));
	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP));
	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE /*D3DTEXF_LINEAR*/));
}

void vmr9_allocator_presenter::destroy_device()
{
	on_device_destroyed();
	device = nullptr;
}

bool vmr9_allocator_presenter::needs_display_change() const
{
	D3DDEVICE_CREATION_PARAMETERS parameters;
	device->GetCreationParameters(&parameters);
	HMONITOR device_monitor(d3d9->get_d3d9()->GetAdapterMonitor(parameters.AdapterOrdinal));
	HMONITOR window_monitor(::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST));

	if(device_monitor != window_monitor)
	{
		D3DADAPTER_IDENTIFIER9 created, current;
		d3d9->get_d3d9()->GetAdapterIdentifier(parameters.AdapterOrdinal, 0, &created);

		for(UINT i = 0, total = d3d9->get_d3d9()->GetAdapterCount(); i < total; ++i)
		{
			if(window_monitor == d3d9->get_d3d9()->GetAdapterMonitor(i))
			{
				d3d9->get_d3d9()->GetAdapterIdentifier(i, 0, &current);
				break;
			}
		}

		dout << "created adaptor: " << created.Description << " " << created.DeviceName << std::endl;
		dout << "current adaptor: " << current.Description << " " << current.DeviceName << std::endl;
	}

	return device_monitor != window_monitor;
}

void vmr9_allocator_presenter::reset_device()
{

#if 1
	D3DPRESENT_PARAMETERS parameters(presentation_parameters);
	HRESULT hr(device->Reset(&parameters));
	switch(hr)
	{
	case S_OK:
		set_device_state();
		on_device_reset();
		dout << "Reset worked" << std::endl;
		break;
	case D3DERR_INVALIDCALL:
		dout << "Still not reset... maybe I'm in the wrong thread.  Recreating from scratch." << std::endl;
		destroy_device();
		create_device();
		break;
	case D3DERR_DEVICELOST:
		on_device_lost();
		break;
	case D3DERR_DEVICENOTRESET:
		dout << "Couldn't reset for some reason or other.  Try later." << std::endl;
		break;
	default:
		dout << "Unknown result: " << std::hex << hr << std::endl;
	}
#else
	destroy_device();
	create_device();
#endif
}

void vmr9_allocator_presenter::reset()
{
	switch(device->CheckDeviceState(window))
	{
	case S_PRESENT_MODE_CHANGED:
		dout << "S_PRESENT_MODE_CHANGED" << std::endl;
		// I can continue rendering if I need to, but it's better to reset and create
		// a new swap chain
		reset_device();
		return;
	case S_PRESENT_OCCLUDED:
		dout << "S_PRESENT_OCCLUDED" << std::endl;
		// I could pause rendering as the window is invisible
		return;
	case D3DERR_DEVICELOST:
		dout << "D3DERR_DEVICELOST" << std::endl;
		reset_device();
		return;
	case D3DERR_DEVICEHUNG:
		dout << "D3DERR_DEVICEHUNG" << std::endl;
		reset_device();
		return;
	case D3DERR_DEVICEREMOVED:
		dout << "D3DERR_DEVICEREMOVED" << std::endl;
		reset_device();
		return;
	case S_OK:
		return;
	}
}
