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
#include "text.h"

Player::Player() : ui(this),
                   event_thread(0),
                   render_thread(0),
                   user_id(0xabcd),
                   allocator(NULL),
                   state(unloaded),
                   fullscreen(false),
                   has_video(false),
                   playlist(),
                   playlist_position(playlist.begin()),
                   playlist_mode(normal),
                   ar_mode(original),
                   wnd_size_mode(one_hundred_percent)
{
	SIZE sz = { 640, 480 };
	window_size = sz;
	video_size = sz;
	scene_size = sz;

	IDirect3D9* d3d9(NULL);

	HMODULE d3d9_dll(::GetModuleHandleW(L"d3d9.dll"));
	typedef HRESULT (WINAPI *d3dcreate9ex_proc)(UINT, IDirect3D9Ex**);
	d3dcreate9ex_proc d3dcreate9ex(reinterpret_cast<d3dcreate9ex_proc>(::GetProcAddress(d3d9_dll, "Direct3DCreate9Ex")));
	if(d3dcreate9ex != NULL)
	{
		IDirect3D9Ex* ptr;
		if(S_OK == d3dcreate9ex(D3D_SDK_VERSION, &ptr))
		{
			dout << "Using D3D9Ex" << std::endl;
			d3d9 = ptr;
		}
	}
	if(d3d9 == NULL)
	{
		dout << "Using D3D9" << std::endl;
		d3d9 = ::Direct3DCreate9(D3D_SDK_VERSION);
	}
	if(d3d9 == NULL)
	{
		_com_raise_error(E_FAIL);
	}
	d3d.Attach(d3d9);

	critical_section::lock l(graph_cs);
	FAIL_THROW(graph.CreateInstance(CLSID_FilterGraph));
	FAIL_THROW(graph->QueryInterface(&events));
	HANDLE evt(0);
	FAIL_THROW(events->GetEventHandle(reinterpret_cast<OAEVENT*>(&evt)));
	::DuplicateHandle(::GetCurrentProcess(), evt, ::GetCurrentProcess(), &event, 0, FALSE, DUPLICATE_SAME_ACCESS);
	cancel_event = ::CreateEventW(NULL, TRUE, FALSE, NULL);
	event_thread = utility::CreateThread(NULL, 0, this, &Player::event_thread_proc, static_cast<void*>(0), "Event thread", 0, 0);
}

Player::~Player()
{
	critical_section::lock l(graph_cs);
	if(get_state() != Player::unloaded)
	{
		try
		{
			stop();
			close();
		}
		catch(_com_error&)
		{
		}
	}
	if(event_thread != 0)
	{
		::SetEvent(cancel_event);
		::WaitForSingleObject(event_thread, INFINITE);
		event_thread = 0;
		::CloseHandle(cancel_event);
		cancel_event = 0;
		::CloseHandle(event);
		event = 0;
	}
	events = NULL;
	graph = NULL;
}

void Player::stop()
{
	if(state == unloaded)
	{
		return;
	}
	critical_section::lock l(graph_cs);
	OAFilterState movie_state;
	media_control->GetState(0, &movie_state);
	while(movie_state != State_Stopped)
	{
		FAIL_THROW(media_control->Stop());
		media_control->GetState(0, &movie_state);
	}
	ui.set_on_top(false);
	state = stopped;
}

void Player::play()
{
	if(state == unloaded)
	{
		return;
	}
	critical_section::lock l(graph_cs);
	ui.set_on_top(true);
	FAIL_THROW(media_control->Run());
	state = playing;
}

void Player::pause()
{
	if(state == unloaded)
	{
		return;
	}
	critical_section::lock l(graph_cs);
	if(state == paused)
	{
		play();
	}
	else
	{
		OAFilterState movie_state;
		media_control->GetState(0, &movie_state);
		while(movie_state != State_Paused)
		{
			FAIL_THROW(media_control->Pause());
			media_control->GetState(0, &movie_state);
		}
		state = paused;
	}
}

void Player::ffwd()
{
	if(state == unloaded)
	{
		return;
	}
	critical_section::lock l(graph_cs);
	// TODO
}

void Player::next()
{
	critical_section::lock l(graph_cs);
	if(playlist.empty())
	{
		return;
	}
	Player::status initial_state(state);
	if(initial_state != unloaded)
	{
		stop();
		LONGLONG current(0);
		seeking->SetPositions(&current, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
		close();
	}
	switch(playlist_mode)
	{
	case normal: // stop on wraparound
		{
			bool hit_end(false);
			++playlist_position;
			if(playlist_position == playlist.end())
			{
				hit_end = true;
				playlist_position = playlist.begin();
			}
			load();
			if(initial_state == playing && !hit_end)
			{
				play();
			}
		}
		break;
	case repeat_single: // when the user presses 'next' in repeat single, we move forward normally
	case repeat_all: // continue on wraparound
		{
			++playlist_position;
			if(playlist_position == playlist.end())
			{
				playlist_position = playlist.begin();
			}
			load();
			if(initial_state == playing)
			{
				play();
			}
		}
		break;
	case shuffle:
		break;
	}
}

void Player::rwnd()
{
	if(state == unloaded)
	{
		return;
	}
	critical_section::lock l(graph_cs);
	// TODO
}

void Player::prev()
{

	critical_section::lock l(graph_cs);
	if(playlist.empty())
	{
		return;
	}
	Player::status initial_state(state);
	if(initial_state != unloaded)
	{
		stop();
		LONGLONG current(0);
		seeking->SetPositions(&current, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
		close();
	}
	switch(playlist_mode)
	{
	case normal:
	case repeat_single:
	case repeat_all:
		{
			if(playlist_position == playlist.begin())
			{
				playlist_position = playlist.end();
			}
			--playlist_position;
			load();
			if(initial_state == playing)
			{
				play();
			}
		}
		break;
	case shuffle:
		break;
	}
}

void Player::load()
{
	critical_section::lock l(graph_cs);
	try
	{
		scene->set_filename(*playlist_position);
		state = loading;
		create_graph();
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
}

void Player::close()
{
	if(state == unloaded)
	{
		return;
	}
	critical_section::lock l(graph_cs);
	destroy_graph();
	scene->set_filename(L"");
	state = unloaded;
}

void Player::destroy_graph()
{
	critical_section::lock l(graph_cs);
	unregister_graph();

	IEnumFiltersPtr filtEn;
	graph->EnumFilters(&filtEn);
	for(IBaseFilterPtr flt; S_OK == filtEn->Next(1, &flt, NULL); graph->EnumFilters(&filtEn))
	{
		graph->RemoveFilter(flt);
	}

	vmr_surface_allocator = NULL;
	allocator = NULL;
	media_control = NULL;
	seeking = NULL;
	vmr9 = NULL;
	audio = NULL;
	video = NULL;

	set_render_fps(25);

	has_video = false;
}

void Player::set_allocator_presenter(IBaseFilterPtr filter, HWND window)
{
	critical_section::lock l(graph_cs);
	IVMRSurfaceAllocatorNotify9Ptr surface_allocator_notify;
	FAIL_THROW(filter->QueryInterface(&surface_allocator_notify));

	allocator = new surface_allocator(this, device);
	vmr_surface_allocator.Attach(allocator, true);

	FAIL_THROW(surface_allocator_notify->AdviseSurfaceAllocator(user_id, vmr_surface_allocator));
	FAIL_THROW(vmr_surface_allocator->AdviseNotify(surface_allocator_notify));
	FAIL_THROW(surface_allocator_notify->SetD3DDevice(device, ::MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY)));
}

REFERENCE_TIME Player::get_average_frame_time(IFilterGraphPtr grph) const
{
	IEnumFiltersPtr filtEn;
	grph->EnumFilters(&filtEn);
	for(IBaseFilterPtr flt; S_OK == filtEn->Next(1, &flt, NULL);)
	{
		IEnumPinsPtr pinEn;
		flt->EnumPins(&pinEn);
		for(IPinPtr pin; S_OK == pinEn->Next(1, &pin, NULL);)
		{
			AM_MEDIA_TYPE mt;
			pin->ConnectionMediaType(&mt);
			ON_BLOCK_EXIT(&FreeMediaType, Loki::ByRef(mt));
			if(mt.majortype == MEDIATYPE_Video)
			{
				REFERENCE_TIME averageFrameTime(0);
				if(mt.formattype == FORMAT_MPEGVideo)
				{
					MPEG1VIDEOINFO* info(reinterpret_cast<MPEG1VIDEOINFO*>(mt.pbFormat));
					averageFrameTime = info->hdr.AvgTimePerFrame;
				}
				else if(mt.formattype == FORMAT_MPEG2Video)
				{
					MPEG2VIDEOINFO* info(reinterpret_cast<MPEG2VIDEOINFO*>(mt.pbFormat));
					averageFrameTime = info->hdr.AvgTimePerFrame;
				}
				else if(mt.formattype == FORMAT_VideoInfo)
				{
					VIDEOINFOHEADER* info(reinterpret_cast<VIDEOINFOHEADER*>(mt.pbFormat));
					averageFrameTime = info->AvgTimePerFrame;
				}
				else if(mt.formattype == FORMAT_VideoInfo2)
				{
					VIDEOINFOHEADER2* info(reinterpret_cast<VIDEOINFOHEADER2*>(mt.pbFormat));
					averageFrameTime = info->AvgTimePerFrame;
				}
				if(averageFrameTime != 0)
				{
					return averageFrameTime;
				}
			}
		}
	}
	return 0;
}


SIZE Player::get_video_size(IFilterGraphPtr grph) const
{
	IEnumFiltersPtr filtEn;
	grph->EnumFilters(&filtEn);
	for(IBaseFilterPtr flt; S_OK == filtEn->Next(1, &flt, NULL);)
	{
		IEnumPinsPtr pinEn;
		flt->EnumPins(&pinEn);
		for(IPinPtr pin; S_OK == pinEn->Next(1, &pin, NULL);)
		{
			AM_MEDIA_TYPE mt;
			pin->ConnectionMediaType(&mt);
			ON_BLOCK_EXIT(&FreeMediaType, Loki::ByRef(mt));
			if(mt.majortype == MEDIATYPE_Video)
			{
				SIZE sz = {0};
				if(mt.formattype == FORMAT_MPEGVideo)
				{
					MPEG1VIDEOINFO* info(reinterpret_cast<MPEG1VIDEOINFO*>(mt.pbFormat));
					sz.cx = info->hdr.bmiHeader.biWidth;
					sz.cy = info->hdr.bmiHeader.biHeight;
				}
				else if(mt.formattype == FORMAT_MPEG2Video)
				{
					MPEG2VIDEOINFO* info(reinterpret_cast<MPEG2VIDEOINFO*>(mt.pbFormat));
					sz.cy = info->hdr.bmiHeader.biHeight;
					sz.cx = (sz.cy * info->hdr.dwPictAspectRatioX) / info->hdr.dwPictAspectRatioY;
				}
				else if(mt.formattype == FORMAT_VideoInfo)
				{
					VIDEOINFOHEADER* info(reinterpret_cast<VIDEOINFOHEADER*>(mt.pbFormat));
					sz.cx = info->bmiHeader.biWidth;
					sz.cy = info->bmiHeader.biHeight;
				}
				else if(mt.formattype == FORMAT_VideoInfo2)
				{
					VIDEOINFOHEADER2* info(reinterpret_cast<VIDEOINFOHEADER2*>(mt.pbFormat));
					sz.cy = info->bmiHeader.biHeight;
					sz.cx = (sz.cy * info->dwPictAspectRatioX) / info->dwPictAspectRatioY;
				}
				if(sz.cx != 0 && sz.cy != 0)
				{
					return sz;
				}
			}
		}
	}
	derr << "Warning: no size detected" << std::endl;
	return get_video_dimensions();
}

void Player::create_graph()
{
	FAIL_THROW(vmr9.CreateInstance(CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER));
	IVMRFilterConfig9Ptr filter_config;
	FAIL_THROW(vmr9->QueryInterface(&filter_config));
	FAIL_THROW(filter_config->SetRenderingMode(VMR9Mode_Renderless));
	set_allocator_presenter(vmr9, ui.get_window());
	// this sets mixing mode
	//FAIL_THROW(filter_config->SetNumberOfStreams(1));
	//IVMRMixerControl9Ptr mixer_control;
	//FAIL_THROW(vmr9->QueryInterface(&mixer_control));
	//DWORD mixing_prefs(0);
	//FAIL_THROW(mixer_control->GetMixingPrefs(&mixing_prefs));
	//mixing_prefs &= ~MixerPref9_RenderTargetMask;
	//mixing_prefs |= MixerPref9_RenderTargetYUV;
	//FAIL_THROW(mixer_control->SetMixingPrefs(mixing_prefs));

	FAIL_THROW(graph->AddFilter(vmr9, L"Video Mixing Renderer 9"));

	_bstr_t current_item(playlist_position->c_str());
	const HRESULT render_result(graph->RenderFile(current_item, NULL));
	switch(render_result)
	{
	case S_OK:
		break;
	case VFW_S_AUDIO_NOT_RENDERED:
		dout << "Partial success: The audio was not rendered." << std::endl;
		break;
	case VFW_S_DUPLICATE_NAME:
		dout << "Success: The Filter Graph Manager modified the filter name to avoid duplication." << std::endl;
		break;
	case VFW_S_PARTIAL_RENDER:
		dout << "Partial success: Some of the streams in this movie are in an unsupported format." << std::endl;
		break;
	case VFW_S_VIDEO_NOT_RENDERED:
		dout << "Partial success: The video was not rendered." << std::endl;
		break;
	case VFW_E_CANNOT_CONNECT:
		dout << "Failure: No combination of intermediate filters could be found to make the connection." << std::endl;
		break;
	case VFW_E_CANNOT_LOAD_SOURCE_FILTER:
		dout << "Failure: The source filter for this file could not be loaded." << std::endl;
		break;
	case VFW_E_CANNOT_RENDER:
		dout << "Failure: No combination of filters could be found to render the stream." << std::endl;
		break;
	case VFW_E_INVALID_FILE_FORMAT:
		dout << "Failure: The file format is invalid." << std::endl;
		break;
	case VFW_E_NOT_FOUND:
		dout << "Failure: An object or name was not found." << std::endl;
		break;
	case VFW_E_UNKNOWN_FILE_TYPE:
		dout << "Failure: The media type of this file is not recognized." << std::endl;
		break;
	case VFW_E_UNSUPPORTED_STREAM:
		dout << "Failure: Cannot play back the file: the format is not supported." << std::endl;
		break;
	case E_ABORT:
	case E_FAIL:
	case E_INVALIDARG:
	case E_OUTOFMEMORY:
	case E_POINTER:
		_com_raise_error(render_result);
		break;
	default:
		dout << "Unknown error: " << std::hex << render_result << std::dec << std::endl;
	}
	register_graph(graph);

	FAIL_THROW(graph->QueryInterface(&media_control));
	FAIL_THROW(graph->QueryInterface(&seeking));
	set_playback_position(0.0f);
	FAIL_THROW(graph->QueryInterface(&audio));
	FAIL_THROW(graph->QueryInterface(&video));

	IEnumPinsPtr pinEn;
	vmr9->EnumPins(&pinEn);
	for(IPinPtr pin; S_OK == pinEn->Next(1, &pin, NULL);)
	{
		AM_MEDIA_TYPE mt;
		pin->ConnectionMediaType(&mt);
		ON_BLOCK_EXIT(&FreeMediaType, Loki::ByRef(mt));
		has_video = has_video || (mt.formattype != FORMAT_None) && (mt.formattype != GUID_NULL);
	}

	set_video_dimensions(get_video_size(graph));

	REFERENCE_TIME average_frame_time(get_average_frame_time(graph));

	double frame_rate(0 == average_frame_time ? 25 : 10000000 / static_cast<double>(average_frame_time));
	set_render_fps(static_cast<unsigned int>(frame_rate));

	state = stopped;

	pause();
}

void Player::register_graph(IUnknownPtr unknown)
{
	IRunningObjectTablePtr rot;
	FAIL_THROW(::GetRunningObjectTable(0, &rot));

	std::wstringstream wss;
	// graphedit looks for ROT objects named "FilterGraph".
	wss << L"FilterGraph " << reinterpret_cast<void*>(unknown.GetInterfacePtr()) << L" pid " << std::hex << ::GetCurrentProcessId() << std::endl;

	IMonikerPtr moniker;
	FAIL_THROW(::CreateItemMoniker(L"!", wss.str().c_str(), &moniker));
	FAIL_THROW(rot->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, unknown, moniker, &rot_key));
}

void Player::unregister_graph(void)
{
	IRunningObjectTablePtr rot;
	FAIL_THROW(::GetRunningObjectTable(0, &rot));
	rot->Revoke(rot_key);
}

::tm Player::convert_win32_time(LONGLONG w32Time)
{
	::tm t = {0};
	t.tm_isdst = 0;
	w32Time /= 10 * 1000 * 1000; // 10,000,000 win32 ticks per second
	t.tm_sec = static_cast<int>(w32Time % 60);
	w32Time /= 60;
	t.tm_min = static_cast<int>(w32Time % 60);
	w32Time /= 60;
	t.tm_hour = static_cast<int>(w32Time);
	return t;
}

void Player::create_ui(int cmd_show)
{
	ui.create_window(cmd_show);
	scene.reset(new player_scene(this, &ui));
	ui.add_message_handler(scene.get());
}

void Player::create_device()
{
	UINT device_ordinal(0);
	for(UINT ord(0); ord < d3d->GetAdapterCount(); ++ord)
	{
		if(d3d->GetAdapterMonitor(ord) == ::MonitorFromWindow(ui.get_window(), MONITOR_DEFAULTTONEAREST))
		{
			device_ordinal = ord;
			break;
		}
	}

	D3DDISPLAYMODE dm;
	FAIL_THROW(d3d->GetAdapterDisplayMode(device_ordinal, &dm));

	std::memset(&presentation_parameters, 0, sizeof(D3DPRESENT_PARAMETERS));
	presentation_parameters.Windowed = TRUE;
	presentation_parameters.hDeviceWindow = ui.get_window();
	presentation_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentation_parameters.BackBufferFormat = dm.Format;
	presentation_parameters.BackBufferHeight = dm.Height;
	presentation_parameters.BackBufferWidth = dm.Width;
	presentation_parameters.BackBufferCount = 1;
	presentation_parameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	presentation_parameters.Flags = D3DPRESENTFLAG_VIDEO;
	presentation_parameters.MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;
	presentation_parameters.EnableAutoDepthStencil = TRUE;
	presentation_parameters.AutoDepthStencilFormat = D3DFMT_D16;

	DWORD qualityLevels(0);
	d3d->CheckDeviceMultiSampleType(device_ordinal, D3DDEVTYPE_HAL, dm.Format, presentation_parameters.Windowed, presentation_parameters.MultiSampleType, &qualityLevels);
	presentation_parameters.MultiSampleQuality = qualityLevels - 1;

	FAIL_THROW(d3d->CreateDevice(device_ordinal, D3DDEVTYPE_HAL, ui.get_window(), D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_NOWINDOWCHANGES, &presentation_parameters, &device));

	FAIL_THROW(device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW));
	FAIL_THROW(device->SetRenderState(D3DRS_LIGHTING, FALSE));
	FAIL_THROW(device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
	FAIL_THROW(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	FAIL_THROW(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
	FAIL_THROW(device->SetRenderState(D3DRS_ZENABLE, TRUE));
	FAIL_THROW(device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE));

	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP));
	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP));
	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));

	scene->on_device_created(device);
	scene->on_device_reset();
}

void Player::destroy_device()
{
	scene->on_device_destroyed();
	device = NULL;
}

void Player::reset_device()
{
	critical_section::lock l(graph_cs);
	if(allocator)
	{
		allocator->begin_device_loss();
	}
	scene->on_device_lost();

#if 0
	// this doesn't seem to work satisfactorily; it says it resets OK, but nothing works.
	D3DPRESENT_PARAMETERS parameters(presentation_parameters);
	HRESULT hr(device->Reset(&parameters));
	switch(hr)
	{
	case S_OK:
		dout << "Reset worked" << std::endl;
		break;
	case D3DERR_INVALIDCALL:
		dout << "Still not reset... maybe I'm in the wrong thread.  Recreating from scratch." << std::endl;
		destroy_device();
		create_device();
		break;
	case D3DERR_DEVICELOST:
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

	scene->on_device_reset();
	if(allocator)
	{
		allocator->end_device_loss(device);
	}
}

int Player::run_ui()
{
	render_timer = ::CreateWaitableTimer(NULL, FALSE, NULL);
	set_render_fps(25);
	cancel_render = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	render_thread = utility::CreateThread(NULL, 0, this, &Player::render_thread_proc, static_cast<void*>(0), "Render Thread", 0, 0);
	int rv(ui.pump_messages());
	if(render_thread != 0)
	{
		::SetEvent(cancel_render);
		::WaitForSingleObject(render_thread, INFINITE);
		::CloseHandle(cancel_render);
		cancel_render = 0;
		::CloseHandle(render_timer);
		render_timer = 0;
		render_thread = 0;
	}

	return rv;
}

bool Player::needs_display_change() const
{
	D3DDEVICE_CREATION_PARAMETERS parameters;
	device->GetCreationParameters(&parameters);
	HMONITOR device_monitor(d3d->GetAdapterMonitor(parameters.AdapterOrdinal));
	HMONITOR window_monitor(::MonitorFromWindow(ui.get_window(), MONITOR_DEFAULTTONEAREST));
	return device_monitor != window_monitor;
}

void Player::reset()
{
	switch(device->TestCooperativeLevel())
	{
	case D3DERR_DRIVERINTERNALERROR:
		dout << "D3DERR_DRIVERINTERNALERROR" << std::endl;
		return;
	case D3DERR_DEVICELOST:
		dout << "D3DERR_DEVICELOST" << std::endl;
		return;
	case D3DERR_DEVICENOTRESET:
		dout << "D3DERR_DEVICENOTRESET" << std::endl;
		reset_device();
		return;
	}
}

void Player::render()
{
	try
	{
		{
			critical_section::lock l(graph_cs);
			// any property that the scene requires that also requires the graph lock
			// must be passed in now, and cannot be retrieved later
			scene->set_video_texture(has_video ? allocator->get_video_texture(user_id) : NULL);
			scene->set_volume(get_linear_volume());
			scene->set_playback_position(get_playback_position());
		}

		IDirect3DSurface9Ptr back_buffer;
		FAIL_THROW(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer));
		FAIL_THROW(device->SetRenderTarget(0, back_buffer));
		FAIL_THROW(device->Clear(0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xff, 0, 0, 0), 1.0f, 0L));
		{
			FAIL_THROW(device->BeginScene());
			ON_BLOCK_EXIT_OBJ(*device, &IDirect3DDevice9::EndScene);
			D3DXMATRIX ortho2D;
			D3DXMatrixOrthoLH(&ortho2D, static_cast<float>(window_size.cx), static_cast<float>(window_size.cy), -1000.0f, 1000.0f);
			FAIL_THROW(device->SetTransform(D3DTS_PROJECTION, &ortho2D));
			scene->render();
		}
		FAIL_THROW(device->Present(NULL, NULL, NULL, NULL));
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
}

DWORD Player::render_thread_proc(void*)
{
	try
	{
		create_device();
		HANDLE evts[] = { render_timer, cancel_render };
		while(::WaitForMultipleObjects(2, evts, FALSE, INFINITE) == WAIT_OBJECT_0)
		{
			try
			{
				if(needs_display_change())
				{
					reset_device();
				}
				reset();
				if(allocator != NULL && allocator->rendering(user_id))
				{
					critical_section::lock l(allocator->get_cs(user_id));
					render();
				}
				else
				{
					render();
				}
			}
			catch(_com_error& ce)
			{
				derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
			}
		}
		destroy_device();
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
	return 0;
}

DWORD Player::event_thread_proc(void*)
{
	HANDLE evts[] = { event, cancel_event };
	while(::WaitForMultipleObjects(2, evts, FALSE, INFINITE) == WAIT_OBJECT_0)
	{
		long eventCode;
		LONG_PTR param1;
		LONG_PTR param2;
		FAIL_THROW(events->GetEvent(&eventCode, &param1, &param2, INFINITE));
		ON_BLOCK_EXIT_OBJ(*events, &IMediaEvent::FreeEventParams, eventCode, param1, param2);
		switch(eventCode)
		{
		case EC_COMPLETE:
			{
				//dout << "EC_COMPLETE" << std::endl;
				Player::status initial_state(state);
				stop();
				LONGLONG current(0);
				seeking->SetPositions(&current, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
				close();
				switch(playlist_mode)
				{
				case normal: // stop on wraparound
					{
						bool hit_end(false);
						++playlist_position;
						if(playlist_position == playlist.end())
						{
							hit_end = true;
							playlist_position = playlist.begin();
						}
						load();
						if(initial_state == playing && !hit_end)
						{
							play();
						}
					}
					break;
				case repeat_single: // when the user presses 'next' in repeat single, we move forward normally
					{
						load();
						if(initial_state == playing)
						{
							play();
						}
					}
					break;
				case repeat_all: // continue on wraparound
					{
						++playlist_position;
						if(playlist_position == playlist.end())
						{
							playlist_position = playlist.begin();
						}
						load();
						if(initial_state == playing)
						{
							play();
						}
					}
					break;
				case shuffle:
					break;
				}
			}
			break;
		//// ( HRESULT, void ) : defaulted (special)
		//case EC_USERABORT: dout << "EC_USERABORT" << std::endl; break;
		//// ( void, void ) : application
		//case EC_ERRORABORT: dout << "EC_ERRORABORT" << std::endl; break;
		//// ( HRESULT, void ) : application
		//case EC_TIME: dout << "EC_TIME" << std::endl; break;
		//// ( DWORD, DWORD ) : application
		//case EC_REPAINT: dout << "EC_REPAINT" << std::endl; break;
		//// ( IPin * (could be NULL), void ) : defaulted
		//case EC_STREAM_ERROR_STOPPED: dout << "EC_STREAM_ERROR_STOPPED" << std::endl; break;
		//case EC_STREAM_ERROR_STILLPLAYING: dout << "EC_STREAM_ERROR_STILLPLAYING" << std::endl; break;
		//// ( HRESULT, DWORD ) : application
		//case EC_ERROR_STILLPLAYING: dout << "EC_ERROR_STILLPLAYING" << std::endl; break;
		//// ( HRESULT, void ) : application
		//case EC_PALETTE_CHANGED: dout << "EC_PALETTE_CHANGED" << std::endl; break;
		//// ( void, void ) : application
		//case EC_VIDEO_SIZE_CHANGED: dout << "EC_VIDEO_SIZE_CHANGED" << std::endl; break;
		//// ( DWORD, void ) : application
		//// LOWORD of the DWORD is the new width, HIWORD is the new height.
		//case EC_QUALITY_CHANGE: dout << "EC_QUALITY_CHANGE" << std::endl; break;
		//// ( void, void ) : application
		//case EC_SHUTTING_DOWN: dout << "EC_SHUTTING_DOWN" << std::endl; break;
		//// ( void, void ) : internal
		//case EC_CLOCK_CHANGED: dout << "EC_CLOCK_CHANGED" << std::endl; break;
		//// ( void, void ) : application
		//case EC_PAUSED: dout << "EC_PAUSED" << std::endl; break;
		//// ( HRESULT, void ) : application
		//case EC_OPENING_FILE: dout << "EC_OPENING_FILE" << std::endl; break;
		//case EC_BUFFERING_DATA: dout << "EC_BUFFERING_DATA" << std::endl; break;
		//// ( BOOL, void ) : application
		//case EC_FULLSCREEN_LOST: dout << "EC_FULLSCREEN_LOST" << std::endl; break;
		//// ( void, IBaseFilter * ) : application
		//case EC_ACTIVATE: dout << "EC_ACTIVATE" << std::endl; break;
		//// ( BOOL, IBaseFilter * ) : internal
		//case EC_NEED_RESTART: dout << "EC_NEED_RESTART" << std::endl; break;
		//// ( void, void ) : defaulted
		//case EC_WINDOW_DESTROYED: dout << "EC_WINDOW_DESTROYED" << std::endl; break;
		//// ( IBaseFilter *, void ) : internal
		//case EC_DISPLAY_CHANGED: dout << "EC_DISPLAY_CHANGED" << std::endl; break;
		//// ( IPin *, void ) : internal
		//case EC_STARVATION: dout << "EC_STARVATION" << std::endl; break;
		//// ( void, void ) : defaulted
		//case EC_OLE_EVENT: dout << "EC_OLE_EVENT" << std::endl; break;
		//// ( BSTR, BSTR ) : application
		//case EC_NOTIFY_WINDOW: dout << "EC_NOTIFY_WINDOW" << std::endl; break;
		//// ( HWND, void ) : internal
		//case EC_STREAM_CONTROL_STOPPED: dout << "EC_STREAM_CONTROL_STOPPED" << std::endl; break;
		//// ( IPin * pSender, DWORD dwCookie )
		//case EC_STREAM_CONTROL_STARTED: dout << "EC_STREAM_CONTROL_STARTED" << std::endl; break;
		//// ( IPin * pSender, DWORD dwCookie )
		//case EC_END_OF_SEGMENT: dout << "EC_END_OF_SEGMENT" << std::endl; break;
		//// ( const REFERENCE_TIME *pStreamTimeAtEndOfSegment, DWORD dwSegmentNumber )
		//case EC_SEGMENT_STARTED: dout << "EC_SEGMENT_STARTED" << std::endl; break;
		//// ( const REFERENCE_TIME *pStreamTimeAtStartOfSegment, DWORD dwSegmentNumber)
		//case EC_LENGTH_CHANGED: dout << "EC_LENGTH_CHANGED" << std::endl; break;
		//// (void, void)
		//case EC_DEVICE_LOST: dout << "EC_DEVICE_LOST" << std::endl; break;
		//// (IUnknown, 0)
		//case EC_STEP_COMPLETE: dout << "EC_STEP_COMPLETE" << std::endl; break;
		//// (BOOL bCacelled, void)
		//case EC_TIMECODE_AVAILABLE: dout << "EC_TIMECODE_AVAILABLE" << std::endl; break;
		//// Param1 has a pointer to the sending object
		//// Param2 has the device ID of the sending object
		//case EC_EXTDEVICE_MODE_CHANGE: dout << "EC_EXTDEVICE_MODE_CHANGE" << std::endl; break;
		//// Param1 has the new mode
		//// Param2 has the device ID of the sending object
		//case EC_STATE_CHANGE: dout << "EC_STATE_CHANGE" << std::endl; break;
		//// ( FILTER_STATE, BOOL bInternal)
		//case EC_GRAPH_CHANGED: dout << "EC_GRAPH_CHANGED" << std::endl; break;
		//// Sent by filter to notify interesting graph changes
		//case EC_CLOCK_UNSET: dout << "EC_CLOCK_UNSET" << std::endl; break;
		//// ( void, void ) : application
		//case EC_VMR_RENDERDEVICE_SET: dout << "EC_VMR_RENDERDEVICE_SET" << std::endl; break;
		//// (Render_Device type, void)
		//case EC_VMR_SURFACE_FLIPPED: dout << "EC_VMR_SURFACE_FLIPPED" << std::endl; break;
		//// (hr - Flip return code, void)
		//case EC_VMR_RECONNECTION_FAILED: dout << "EC_VMR_RECONNECTION_FAILED" << std::endl; break;
		//// (hr - ReceiveConnection return code, void)
		//case EC_PREPROCESS_COMPLETE: dout << "EC_PREPROCESS_COMPLETE" << std::endl; break;
		//// Param1 = 0, Param2 = IBaseFilter ptr of sending filter
		//case EC_CODECAPI_EVENT: dout << "EC_CODECAPI_EVENT" << std::endl; break;
		//// Param1 = UserDataPointer, Param2 = VOID* Data
		//case EC_WMT_INDEX_EVENT: dout << "EC_WMT_INDEX_EVENT" << std::endl; break;
		//// lParam1 is one of the enum WMT_STATUS messages listed below, sent by the WindowsMedia SDK
		//// lParam2 is specific to the lParam event
		//case EC_WMT_EVENT: dout << "EC_WMT_EVENT" << std::endl; break;
		//// lParam1 is one of the enum WMT_STATUS messages listed below, sent by the WindowsMedia SDK
		//// lParam2 is a pointer an AM_WMT_EVENT_DATA structure where,
		////                          hrStatus is the status code sent by the wmsdk
		////                          pData is specific to the lParam1 event
		//case EC_BUILT: dout << "EC_BUILT" << std::endl; break;
		//// Sent to notify transition from unbuilt to built state
		//case EC_UNBUILT: dout << "EC_UNBUILT" << std::endl; break;
		//// Sent to notify transtion from built to unbuilt state
		//case EC_DVD_DOMAIN_CHANGE: dout << "EC_DVD_DOMAIN_CHANGE" << std::endl; break;
		//// Parameters: ( DWORD, void ) 
		//case EC_DVD_TITLE_CHANGE: dout << "EC_DVD_TITLE_CHANGE" << std::endl; break;
		//// Parameters: ( DWORD, void ) 
		//case EC_DVD_CHAPTER_START: dout << "EC_DVD_CHAPTER_START" << std::endl; break;
		//// Parameters: ( DWORD, void ) 
		//case EC_DVD_AUDIO_STREAM_CHANGE: dout << "EC_DVD_AUDIO_STREAM_CHANGE" << std::endl; break;
		//// Parameters: ( DWORD, void ) 
		//case EC_DVD_SUBPICTURE_STREAM_CHANGE: dout << "EC_DVD_SUBPICTURE_STREAM_CHANGE" << std::endl; break;
		//// Parameters: ( DWORD, BOOL ) 
		//case EC_DVD_ANGLE_CHANGE: dout << "EC_DVD_ANGLE_CHANGE" << std::endl; break;
		//// Parameters: ( DWORD, DWORD ) 
		//case EC_DVD_BUTTON_CHANGE: dout << "EC_DVD_BUTTON_CHANGE" << std::endl; break;
		//// Parameters: ( DWORD, DWORD ) 
		//case EC_DVD_VALID_UOPS_CHANGE: dout << "EC_DVD_VALID_UOPS_CHANGE" << std::endl; break;
		//// Parameters: ( DWORD, void ) 
		//case EC_DVD_STILL_ON: dout << "EC_DVD_STILL_ON" << std::endl; break;
		//// Parameters: ( BOOL, DWORD ) 
		//case EC_DVD_STILL_OFF: dout << "EC_DVD_STILL_OFF" << std::endl; break;
		//// Parameters: ( void, void ) 
		//case EC_DVD_CURRENT_TIME: dout << "EC_DVD_CURRENT_TIME" << std::endl; break;
		//// Parameters: ( DWORD, BOOL ) 
		//case EC_DVD_ERROR: dout << "EC_DVD_ERROR" << std::endl; break;
		//// Parameters: ( DWORD, void) 
		//case EC_DVD_WARNING: dout << "EC_DVD_WARNING" << std::endl; break;
		//// Parameters: ( DWORD, DWORD) 
		//case EC_DVD_CHAPTER_AUTOSTOP: dout << "EC_DVD_CHAPTER_AUTOSTOP" << std::endl; break;
		//// Parameters: (BOOL, void)
		//case EC_DVD_NO_FP_PGC: dout << "EC_DVD_NO_FP_PGC" << std::endl; break;
		////  Parameters : (void, void)
		//case EC_DVD_PLAYBACK_RATE_CHANGE: dout << "EC_DVD_PLAYBACK_RATE_CHANGE" << std::endl; break;
		////  Parameters : (LONG, void)
		//case EC_DVD_PARENTAL_LEVEL_CHANGE: dout << "EC_DVD_PARENTAL_LEVEL_CHANGE" << std::endl; break;
		////  Parameters : (LONG, void)
		//case EC_DVD_PLAYBACK_STOPPED: dout << "EC_DVD_PLAYBACK_STOPPED" << std::endl; break;
		////  Parameters : (DWORD, void)
		//case EC_DVD_ANGLES_AVAILABLE: dout << "EC_DVD_ANGLES_AVAILABLE" << std::endl; break;
		////  Parameters : (BOOL, void)
		//case EC_DVD_PLAYPERIOD_AUTOSTOP: dout << "EC_DVD_PLAYPERIOD_AUTOSTOP" << std::endl; break;
		//// Parameters: (void, void)
		//case EC_DVD_BUTTON_AUTO_ACTIVATED: dout << "EC_DVD_BUTTON_AUTO_ACTIVATED" << std::endl; break;
		//// Parameters: (DWORD button, void)
		//case EC_DVD_CMD_START: dout << "EC_DVD_CMD_START" << std::endl; break;
		//// Parameters: (CmdID, HRESULT)
		//case EC_DVD_CMD_END: dout << "EC_DVD_CMD_END" << std::endl; break;
		//// Parameters: (CmdID, HRESULT)
		//case EC_DVD_DISC_EJECTED: dout << "EC_DVD_DISC_EJECTED" << std::endl; break;
		//// Parameters: none
		//case EC_DVD_DISC_INSERTED: dout << "EC_DVD_DISC_INSERTED" << std::endl; break;
		//// Parameters: none
		//case EC_DVD_CURRENT_HMSF_TIME: dout << "EC_DVD_CURRENT_HMSF_TIME" << std::endl; break;
		//// Parameters: ( ULONG, ULONG ) 
		//case EC_DVD_KARAOKE_MODE: dout << "EC_DVD_KARAOKE_MODE" << std::endl; break;
		//// Parameters: ( BOOL, reserved ) 
		//default:
		//	dout << "unknown event: " << eventCode << std::endl;
		}
	}
	return 0;
}
