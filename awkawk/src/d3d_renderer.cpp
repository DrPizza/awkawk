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

#include "d3d_renderer.h"
#include "awkawk.h"
#include "player_direct_show.h"
#include "shared_texture_queue.h"

d3d_renderer::d3d_renderer(awkawk* player_,
                           shared_texture_queue* texture_queue_,
                           HWND window_) : direct3d_manager(nullptr),
                                           cs("d3d_renderer"),
                                           player(player_),
                                           texture_queue(texture_queue_),
                                           window(window_),
                                           render_thread(0) {
}

d3d_renderer::~d3d_renderer() {
	stop_rendering();
}

void d3d_renderer::start_rendering() {
	render_timer = ::CreateWaitableTimerW(nullptr, TRUE, nullptr);
	set_render_fps(25);
	render_event = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
	render_and_wait_event = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
	render_finished_event = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
	cancel_render = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
	render_thread = utility::CreateThread(nullptr, 0, this, &d3d_renderer::render_thread_proc, nullptr, "Render Thread", 0, nullptr);
	schedule_render();
}

void d3d_renderer::stop_rendering() {
	if(render_thread != 0) {
		::SetEvent(cancel_render);
		::WaitForSingleObject(render_thread, INFINITE);
		::CloseHandle(render_finished_event);
		::CloseHandle(render_and_wait_event);
		::CloseHandle(render_event);
		::CloseHandle(cancel_render);
		::CloseHandle(render_thread);
		::CloseHandle(render_timer);
		render_thread = 0;
	}
}

void d3d_renderer::create_d3d() {
	d3d9.reset(new direct3d9());
	DXVA2CreateDirect3DDeviceManager9(&reset_token, &device_manager);
}

void d3d_renderer::destroy_d3d() {
	d3d9.reset();
}

void d3d_renderer::create_device()
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

void d3d_renderer::set_device_state()
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

void d3d_renderer::destroy_device()
{
	on_device_destroyed();
	device = nullptr;
}

bool d3d_renderer::needs_display_change() const
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

void d3d_renderer::reset_device()
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

DWORD d3d_renderer::render_thread_proc(void*)
{
	try
	{
		FAIL_THROW(::CoInitializeEx(nullptr, COINIT_MULTITHREADED));
		ON_BLOCK_EXIT(::CoUninitialize());

		create_d3d();
		create_device();

		TIMECAPS tc = {0};
		timeGetDevCaps(&tc, sizeof(TIMECAPS));
		timeBeginPeriod(tc.wPeriodMin);
		ON_BLOCK_EXIT(timeEndPeriod(tc.wPeriodMin));

		DWORD task_index(0);
		HANDLE task(::AvSetMmThreadCharacteristics(L"Playback", &task_index));
		ON_BLOCK_EXIT(::AvRevertMmThreadCharacteristics(task));
		::AvSetMmThreadPriority(task, AVRT_PRIORITY_HIGH);

		LARGE_INTEGER previous_render_time = {0};
		LARGE_INTEGER current_time = {0};
		LARGE_INTEGER frequency = {0};
		::QueryPerformanceFrequency(&frequency);

		bool continue_rendering(true);
		while(continue_rendering)
		{
			float frame_delay(1.0f);
			bool is_waiting(false);
			HANDLE evts[] = { render_and_wait_event, render_event, render_timer, cancel_render };
			switch(::WaitForMultipleObjects(ARRAY_SIZE(evts), evts, FALSE, INFINITE))
			{
			case WAIT_OBJECT_0:
				is_waiting = true;
			case WAIT_OBJECT_0 + 1:
				frame_delay = 2.0f;
			case WAIT_OBJECT_0 + 2:
				::QueryPerformanceCounter(&current_time);
				//dout << (static_cast<double>(frequency.QuadPart) / static_cast<double>(current_time.QuadPart - previous_render_time.QuadPart)) << std::endl;
				schedule_render(frame_delay);
				try
				{
					//if(needs_display_change())
					//{
					//	reset_device();
					//}
					reset();
					render();
				}
				catch(_com_error& ce)
				{
					derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
				}
				if(is_waiting)
				{
					::SetEvent(render_finished_event);
				}
				if(texture_queue->backlogged()) {
					signal_new_frame();
				}
				previous_render_time = current_time;
				break;
			case WAIT_OBJECT_0 + 3:
			default:
				continue_rendering = false;
				break;
			}
		}
		destroy_device();
		destroy_d3d();
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
	catch(std::exception& e)
	{
		derr << e.what() << std::endl;
	}
	return 0;
}

void d3d_renderer::reset()
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

void d3d_renderer::do_emit_scene() {
	SIZE window_size(player->get_window_dimensions());
	D3DXMATRIX ortho2D;
	D3DXMatrixOrthoLH(&ortho2D, static_cast<float>(window_size.cx), static_cast<float>(window_size.cy), 0.0f, 1.0f);

	FAIL_THROW(device->SetTransform(D3DTS_PROJECTION, &ortho2D));

	direct3d_manager::do_emit_scene();
}

void d3d_renderer::render()
{
	try
	{
		IDirect3DSurface9Ptr back_buffer;
		device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
		device->SetRenderTarget(0, back_buffer);
		set_device_state();

		static D3DCOLOR col(D3DCOLOR_ARGB(0xff, 0, 0, 0));
#if 0
		col = col == D3DCOLOR_ARGB(0xff, 0xff, 0, 0) ? D3DCOLOR_ARGB(0xff, 0, 0xff, 0)
		    : col == D3DCOLOR_ARGB(0xff, 0, 0xff, 0) ? D3DCOLOR_ARGB(0xff, 0, 0, 0xff)
		                                             : D3DCOLOR_ARGB(0xff, 0xff, 0, 0);
#endif
		//col = D3DCOLOR_ARGB(0xff, 0, 0xff, 0);

		FAIL_THROW(device->Clear(0L, nullptr, D3DCLEAR_TARGET, col, 1.0f, 0));
		{
			FAIL_THROW(device->BeginScene());
			ON_BLOCK_EXIT(device->EndScene());
			emit_scene();
		}
		FAIL_THROW(device->Present(nullptr, nullptr, NULL, nullptr));
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
}
