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

Player::Player() : ui(this), event_thread(0), update_thread(0), render_thread(0), user_id(0xabcd), allocator(NULL), state(unloaded), fullscreen(false), has_video(false), playlist(), playlist_position(playlist.begin()), playlist_mode(normal)
{
	SIZE sz = { 640, 480 };
	window_size = sz;
	video_size = sz;
	scene_size = sz;

	if(!VerifyVMR9())
	{
		throw std::runtime_error("VMR9 not available");
	}
	critical_section::lock l(graph_cs);

	d3d.Attach(::Direct3DCreate9(D3D_SDK_VERSION));
	if(d3d == NULL)
	{
		_com_raise_error(E_FAIL);
	}

	FAIL_THROW(graph.CreateInstance(CLSID_FilterGraph));

	FAIL_THROW(graph->QueryInterface(&events));
	HANDLE evt(0);
	FAIL_THROW(events->GetEventHandle(reinterpret_cast<OAEVENT*>(&evt)));
	::DuplicateHandle(::GetCurrentProcess(), evt, ::GetCurrentProcess(), &event, 0, FALSE, DUPLICATE_SAME_ACCESS);
	cancel_event = ::CreateEvent(NULL, TRUE, FALSE, NULL);
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
	if(update_thread != 0)
	{
		::SetEvent(cancel_update);
		::WaitForSingleObject(update_thread, INFINITE);
		update_thread = 0;
		::CloseHandle(cancel_update);
		cancel_update = 0;
		::CloseHandle(update_timer);
		update_timer = 0;
	}

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
	has_video = false;
	ui.invalidate();
}

void Player::set_allocator_presenter(IBaseFilterPtr filter, HWND window)
{
	critical_section::lock l(graph_cs);
	IVMRSurfaceAllocatorNotify9Ptr lpIVMRSurfAllocNotify;
	FAIL_THROW(filter->QueryInterface(&lpIVMRSurfAllocNotify));

	allocator = new surface_allocator(this, window, device);
	vmr_surface_allocator.Attach(allocator, true);

	FAIL_THROW(lpIVMRSurfAllocNotify->AdviseSurfaceAllocator(user_id, vmr_surface_allocator));
	FAIL_THROW(vmr_surface_allocator->AdviseNotify(lpIVMRSurfAllocNotify));
	FAIL_THROW(lpIVMRSurfAllocNotify->SetD3DDevice(device, ::MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY)));
}

REFERENCE_TIME Player::get_average_frame_time(IFilterGraphPtr grph)
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


void Player::create_graph()
{
	FAIL_THROW(vmr9.CreateInstance(CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER));
	IVMRFilterConfig9Ptr filterConfig;
	FAIL_THROW(vmr9->QueryInterface(&filterConfig));
	FAIL_THROW(filterConfig->SetRenderingMode(VMR9Mode_Renderless));
	set_allocator_presenter(vmr9, ui.get_window());
	// this sets mixing mode
	FAIL_THROW(filterConfig->SetNumberOfStreams(1));
	FAIL_THROW(graph->AddFilter(vmr9, L"Video Mixing Renderer 9"));

	_bstr_t current_item(playlist_position->c_str());
	const HRESULT render_result(graph->RenderFile(current_item, NULL));
	switch(render_result)
	{
	case S_OK: break;
	case VFW_S_AUDIO_NOT_RENDERED:
		dout << "Partial success; the audio was not rendered." << std::endl;
		break;
	case VFW_S_DUPLICATE_NAME:
		dout << "Success; the Filter Graph Manager modified the filter name to avoid duplication." << std::endl;
		break;
	case VFW_S_PARTIAL_RENDER:
		dout << "Some of the streams in this movie are in an unsupported format." << std::endl;
		break;
	case VFW_S_VIDEO_NOT_RENDERED:
		dout << "Partial success; some of the streams in this movie are in an unsupported format." << std::endl;
		break;
	case VFW_E_CANNOT_CONNECT:
		dout << "No combination of intermediate filters could be found to make the connection." << std::endl;
		break;
	case VFW_E_CANNOT_LOAD_SOURCE_FILTER:
		dout << "The source filter for this file could not be loaded." << std::endl;
		break;
	case VFW_E_CANNOT_RENDER:
		dout << "No combination of filters could be found to render the stream." << std::endl;
		break;
	case VFW_E_INVALID_FILE_FORMAT:
		dout << "The file format is invalid." << std::endl;
		break;
	case VFW_E_NOT_FOUND:
		dout << "An object or name was not found." << std::endl;
		break;
	case VFW_E_UNKNOWN_FILE_TYPE:
		dout << "The media type of this file is not recognized." << std::endl;
		break;
	case VFW_E_UNSUPPORTED_STREAM:
		dout << "Cannot play back the file: the format is not supported." << std::endl;
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

	FAIL_THROW(graph->QueryInterface(&media_control));
	FAIL_THROW(graph->QueryInterface(&seeking));
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

	//REFERENCE_TIME avgFrameTime(get_average_frame_time(graph));

	//double frameRate(1e7 / static_cast<double>(avgFrameTime));
	//dout << "framerate: " << frameRate << std::endl;

	state = stopped;
	register_graph(graph);

	update_timer = ::CreateWaitableTimer(NULL, FALSE, NULL);
	LARGE_INTEGER dueTime = {0};
	long period(1000 / 25); // TODO get framerate?  Or maybe refresh rate?
	::SetWaitableTimer(update_timer, &dueTime, period, NULL, NULL, FALSE);
	cancel_update = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	update_thread = utility::CreateThread(NULL, 0, this, &Player::update_thread_proc, static_cast<void*>(0), "Update Thread", 0, 0);
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

DWORD Player::update_thread_proc(void*)
{
	HANDLE evts[] = { update_timer, cancel_update };
	while(::WaitForMultipleObjects(2, evts, FALSE, INFINITE) == WAIT_OBJECT_0)
	{
		// bool increase(false);
		//float volume(get_linear_volume());
		//float log_volume(get_volume());
		//dout << "volume " << volume << " " << log_volume << std::endl;
		//increase = volume >= 1.0f ? false :
		//           volume <= 0.0f ? true : increase;
		//volume += increase ? 0.05f : -0.05f;
		//set_linear_volume(volume);

		//LONGLONG current, duration;
		//seeking->GetCurrentPosition(&current);
		//seeking->GetDuration(&duration);
		//::tm cur(convert_win32_time(current)), dur(convert_win32_time(duration));
		//std::wstringstream wss;
		//wss << utility::settimeformat(L"%H:%M:%S") << cur << L" " << dur;
		//wdout << wss.str() << std::endl;

	}
	return 0;
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

void Player::create_ui(int cmdShow)
{
	ui.create_window(cmdShow);
	create_device();
	scene.reset(new player_scene(this, &ui, device));
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

	D3DPRESENT_PARAMETERS pp = {0};
	pp.Windowed = TRUE;
	pp.hDeviceWindow = ui.get_window();
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.BackBufferFormat = dm.Format;
	pp.BackBufferHeight = dm.Height;
	pp.BackBufferWidth = dm.Width;
	pp.BackBufferCount = 1;
	pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	pp.Flags = D3DPRESENTFLAG_VIDEO;
	pp.MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;

	DWORD qualityLevels(0);
	d3d->CheckDeviceMultiSampleType(device_ordinal, D3DDEVTYPE_HAL, dm.Format, pp.Windowed, pp.MultiSampleType, &qualityLevels);
	pp.MultiSampleQuality = qualityLevels - 1;

	FAIL_THROW(d3d->CreateDevice(device_ordinal, D3DDEVTYPE_HAL, ui.get_window(), D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_NOWINDOWCHANGES, &pp, &device));

	FAIL_THROW(device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
	FAIL_THROW(device->SetRenderState(D3DRS_LIGHTING, FALSE));
	FAIL_THROW(device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
	FAIL_THROW(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	FAIL_THROW(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
	FAIL_THROW(device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE));
	FAIL_THROW(device->SetRenderState(D3DRS_ALPHAREF, 0x10));
	FAIL_THROW(device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER));

	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP));
	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP));
	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
	FAIL_THROW(device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));

	FAIL_THROW(device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE));
	FAIL_THROW(device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE));

	FAIL_THROW(device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE));

	FAIL_THROW(device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE));
	FAIL_THROW(device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE));

	FAIL_THROW(device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE));
}

void Player::destroy_device()
{
	device = NULL;
}

void Player::reset_device()
{
	critical_section::lock l(graph_cs);
	begin_device_loss();
	destroy_device();
	create_device();
	end_device_loss(device);
}

void Player::begin_device_loss()
{
	scene->begin_device_loss();
	if(allocator)
	{
		allocator->begin_device_loss();
	}
}

void Player::end_device_loss(IDirect3DDevice9Ptr device)
{
	scene->end_device_loss(device);
	if(allocator)
	{
		allocator->end_device_loss(device);
	}
}

int Player::run_ui()
{
	render_timer = ::CreateWaitableTimer(NULL, FALSE, NULL);
	LARGE_INTEGER dueTime = { 0 };
	dueTime.QuadPart = -10000000 / 25;
	::SetWaitableTimer(render_timer, &dueTime, 1000 / 25, NULL, NULL, FALSE);
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
		// the reason I only try to obtain the lock is that otherwise I can deadlock when stopping/switching files
		// The WndProc thread calls stop, which takes out the player lock.  VMR9 calls PresentImage, which calls render
		// which also waits on the player lock (and is blocked by the 'stop').  The problem the arises because VMR waits
		// until the PresentImage call returns.  Which it never does, because it's waiting for the player lock, and the 
		// player lock will only be released when the stop succeeds.
		critical_section::attempt_lock l(graph_cs);
		if(l.succeeded)
		{
			if(has_video)
			{
				IDirect3DTexture9Ptr video_texture;
				allocator->get_video_texture(user_id, video_texture);
				scene->set_video_texture(video_texture);
			}
			else
			{
				scene->set_video_texture(NULL);
			}
			// any property that the scene requires that also requires the graph lock
			// must be passed in here
			scene->set_volume(get_linear_volume());
			scene->set_playback_position(get_playback_position());
		}

		IDirect3DSurface9Ptr back_buffer;
		FAIL_THROW(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer));
		FAIL_THROW(device->SetRenderTarget(0, back_buffer));
		FAIL_THROW(device->Clear(0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0L));
		FAIL_THROW(device->BeginScene());
		scene->render();
		FAIL_THROW(device->EndScene());
		FAIL_THROW(device->Present(NULL, NULL, NULL, NULL));
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
}

DWORD Player::render_thread_proc(void*)
{
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
			if(get_state() != playing || !has_video)
			{
				render();
			}
		}
		catch(_com_error& ce)
		{
			derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
		}
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
