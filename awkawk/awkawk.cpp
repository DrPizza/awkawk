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
#include "text.h"
#include "player_direct_show.h"

const awkawk::transition_type awkawk::transitions[awkawk::max_awkawk_states][awkawk::max_awkawk_events] =
{
/* state    | event              | handler               | exit states */
/* ------------------------ */ {
/* unloaded | load          */   { &awkawk::do_load      , awkawk::transition_type::state_array() << awkawk::stopped                                        },
/*          | stop          */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | pause         */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | play          */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | unload        */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | transitioning */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | previous      */   { &awkawk::do_previous  , awkawk::transition_type::state_array() << awkawk::unloaded << awkawk::stopped << awkawk::playing },
/*          | next          */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | rwnd          */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | ffwd          */   { NULL                  , awkawk::transition_type::state_array()                                                           }
                          },
                          {
/* stopped  | load          */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | stop          */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | pause         */   { &awkawk::do_play      , awkawk::transition_type::state_array() << awkawk::playing                                        },
/*          | play          */   { &awkawk::do_play      , awkawk::transition_type::state_array() << awkawk::playing                                        },
/*          | unload        */   { &awkawk::do_unload    , awkawk::transition_type::state_array() << awkawk::unloaded                                       },
/*          | transitioning */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | previous      */   { &awkawk::do_previous  , awkawk::transition_type::state_array() << awkawk::unloaded << awkawk::stopped << awkawk::playing },
/*          | next          */   { &awkawk::do_next      , awkawk::transition_type::state_array() << awkawk::unloaded << awkawk::stopped << awkawk::playing },
/*          | rwnd          */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | ffwd          */   { NULL                  , awkawk::transition_type::state_array()                                                           }
                          },
                          {
/* paused   | load          */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | stop          */   { &awkawk::do_stop      , awkawk::transition_type::state_array() << awkawk::stopped                                        },
/*          | pause         */   { &awkawk::do_resume    , awkawk::transition_type::state_array() << awkawk::playing                                        },
/*          | play          */   { &awkawk::do_resume    , awkawk::transition_type::state_array() << awkawk::playing                                        },
/*          | unload        */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | transitioning */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | previous      */   { &awkawk::do_previous  , awkawk::transition_type::state_array() << awkawk::unloaded << awkawk::stopped << awkawk::playing },
/*          | next          */   { &awkawk::do_next      , awkawk::transition_type::state_array() << awkawk::unloaded << awkawk::stopped << awkawk::playing },
/*          | rwnd          */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | ffwd          */   { NULL                  , awkawk::transition_type::state_array()                                                           }
                          },
                          {
/* playing  | load          */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | stop          */   { &awkawk::do_stop      , awkawk::transition_type::state_array() << awkawk::stopped                                        },
/*          | pause         */   { &awkawk::do_pause     , awkawk::transition_type::state_array() << awkawk::paused                                         },
/*          | play          */   { &awkawk::do_pause     , awkawk::transition_type::state_array() << awkawk::paused                                         },
/*          | unload        */   { NULL                  , awkawk::transition_type::state_array()                                                           },
/*          | transitioning */   { &awkawk::do_transition, awkawk::transition_type::state_array() << awkawk::unloaded << awkawk::stopped << awkawk::playing },
/*          | previous      */   { &awkawk::do_previous  , awkawk::transition_type::state_array() << awkawk::unloaded << awkawk::stopped << awkawk::playing },
/*          | next          */   { &awkawk::do_next      , awkawk::transition_type::state_array() << awkawk::unloaded << awkawk::stopped << awkawk::playing },
/*          | rwnd          */   { &awkawk::do_rwnd      , awkawk::transition_type::state_array() << awkawk::playing                                        },
/*          | ffwd          */   { &awkawk::do_ffwd      , awkawk::transition_type::state_array() << awkawk::playing                                        }
                          }
};

awkawk::awkawk() : ui(new player_window(this)),
                   dshow(new player_direct_show(this)),
                   plist(new player_playlist()),
                   current_state(unloaded),
                   handler(new event_handler_type(this)),
                   render_thread(0),
                   fullscreen(false),
                   chosen_ar(0),
                   chosen_lb(0),
                   wnd_size_mode(one_hundred_percent)
{
	available_ratios.push_back(boost::shared_ptr<aspect_ratio>(new natural_aspect_ratio(this)));
	available_ratios.push_back(boost::shared_ptr<aspect_ratio>(new fixed_aspect_ratio(4, 3)));
	available_ratios.push_back(boost::shared_ptr<aspect_ratio>(new fixed_aspect_ratio(14, 9)));
	available_ratios.push_back(boost::shared_ptr<aspect_ratio>(new fixed_aspect_ratio(16, 9)));
	available_ratios.push_back(boost::shared_ptr<aspect_ratio>(new fixed_aspect_ratio(1.85f)));
	available_ratios.push_back(boost::shared_ptr<aspect_ratio>(new fixed_aspect_ratio(2.40f)));

	available_letterboxes.push_back(boost::shared_ptr<letterbox>(new natural_letterbox(this)));
	available_letterboxes.push_back(boost::shared_ptr<letterbox>(new fixed_letterbox(4, 3)));
	available_letterboxes.push_back(boost::shared_ptr<letterbox>(new fixed_letterbox(14, 9)));
	available_letterboxes.push_back(boost::shared_ptr<letterbox>(new fixed_letterbox(16, 9)));
	available_letterboxes.push_back(boost::shared_ptr<letterbox>(new fixed_letterbox(1.85f)));
	available_letterboxes.push_back(boost::shared_ptr<letterbox>(new fixed_letterbox(2.40f)));

	SIZE sz = { 640, 480 };
	window_size = sz;
	video_size = sz;
	scene_size = sz;
}

awkawk::~awkawk()
{
	dshow.reset();
}

void awkawk::create_d3d()
{
	d3d9.create_d3d();
}

void awkawk::destroy_d3d()
{
	d3d9.destroy_d3d();
}

void awkawk::create_ui(int cmd_show)
{
	ui->create_window(cmd_show);
	scene.reset(new player_scene(this, get_ui()));
	overlay.reset(new player_overlay(this, get_ui()));
	scene->add_components(overlay.get());
}

void awkawk::create_device()
{
#if defined(USE_RGBRAST)
	HMODULE rgb_rast(::LoadLibraryW(L"rgb9rast.dll"));
	if(rgb_rast != NULL)
	{
		FARPROC rgb_rast_register(::GetProcAddress(rgb_rast, "D3D9GetSWInfo"));
		d3d->RegisterSoftwareDevice(reinterpret_cast<void*>(rgb_rast_register));
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
	for(UINT ord(0); ord < d3d9.get_d3d9()->GetAdapterCount(); ++ord)
	{
		if(d3d9.get_d3d9()->GetAdapterMonitor(ord) == ::MonitorFromWindow(get_ui()->get_window(), MONITOR_DEFAULTTONEAREST))
		{
			device_ordinal = ord;
			break;
		}
	}

	D3DDISPLAYMODE dm;
	FAIL_THROW(d3d9.get_d3d9()->GetAdapterDisplayMode(device_ordinal, &dm));

	D3DCAPS9 caps;
	FAIL_THROW(d3d9.get_d3d9()->GetDeviceCaps(device_ordinal, dev_type, &caps));
	if((caps.TextureCaps & D3DPTEXTURECAPS_POW2) && !(caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
	{
		::MessageBoxW(get_ui()->get_window(), L"The device does not support non-power of 2 textures.  awkawk cannot continue.", L"Fatal Error", MB_ICONERROR);
		throw std::runtime_error("The device does not support non-power of 2 textures.  awkawk cannot continue.");
	}
	if(caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
	{
		::MessageBoxW(get_ui()->get_window(), L"The device does not support non-square textures.  awkawk cannot continue.", L"Fatal Error", MB_ICONERROR);
		throw std::runtime_error("The device does not support non-square textures.  awkawk cannot continue.");
	}

	std::memset(&presentation_parameters, 0, sizeof(D3DPRESENT_PARAMETERS));
	//presentation_parameters.BackBufferWidth = dm.Width;
	//presentation_parameters.BackBufferHeight = dm.Height;
	presentation_parameters.BackBufferWidth = std::max(static_cast<UINT>(1920), dm.Width);
	presentation_parameters.BackBufferHeight = std::max(static_cast<UINT>(1080), dm.Height);
	presentation_parameters.BackBufferFormat = dm.Format;
	presentation_parameters.BackBufferCount = 1;
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
	presentation_parameters.hDeviceWindow = get_ui()->get_window();
	presentation_parameters.Windowed = TRUE;
	presentation_parameters.EnableAutoDepthStencil = TRUE;
	presentation_parameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentation_parameters.Flags = D3DPRESENTFLAG_VIDEO;
	presentation_parameters.FullScreen_RefreshRateInHz = 0;
	presentation_parameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	D3DPRESENT_PARAMETERS parameters(presentation_parameters);
	FAIL_THROW(d3d9.get_d3d9()->CreateDevice(device_ordinal, dev_type, get_ui()->get_window(), vertex_processing | D3DCREATE_MULTITHREADED | D3DCREATE_NOWINDOWCHANGES | D3DCREATE_FPU_PRESERVE, &parameters, &scene_device));

	set_device_state();

	FAIL_THROW(dshow->on_device_created(scene_device));
	FAIL_THROW(scene->on_device_created(scene_device));
	FAIL_THROW(overlay->on_device_created(scene_device));
	FAIL_THROW(dshow->on_device_reset());
	FAIL_THROW(scene->on_device_reset());
	FAIL_THROW(overlay->on_device_reset());
}

void awkawk::set_device_state()
{
	FAIL_THROW(scene_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
	FAIL_THROW(scene_device->SetRenderState(D3DRS_LIGHTING, FALSE));
	FAIL_THROW(scene_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
	FAIL_THROW(scene_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	FAIL_THROW(scene_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
	FAIL_THROW(scene_device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE));
	FAIL_THROW(scene_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE));

	FAIL_THROW(scene_device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP));
	FAIL_THROW(scene_device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP));
	FAIL_THROW(scene_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
	FAIL_THROW(scene_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
	FAIL_THROW(scene_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE /*D3DTEXF_LINEAR*/));
}

void awkawk::destroy_device()
{
	dshow->on_device_destroyed();
	scene->on_device_destroyed();
	overlay->on_device_destroyed();
	scene_device = NULL;
}

void awkawk::reset_device()
{
	// TODO This was locked for a reason
	LOCK(dshow->graph_cs);
	FAIL_THROW(dshow->on_device_lost());
	FAIL_THROW(scene->on_device_lost());
	FAIL_THROW(overlay->on_device_lost());

#if 1
	D3DPRESENT_PARAMETERS parameters(presentation_parameters);
	HRESULT hr(scene_device->Reset(&parameters));
	switch(hr)
	{
	case S_OK:
		set_device_state();
		FAIL_THROW(dshow->on_device_reset());
		FAIL_THROW(scene->on_device_reset());
		FAIL_THROW(overlay->on_device_reset());
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
}

int awkawk::run_ui()
{
	render_timer = ::CreateWaitableTimerW(NULL, TRUE, NULL);
	set_render_fps(25);
	render_event = ::CreateEventW(NULL, FALSE, FALSE, NULL);
	cancel_render = ::CreateEventW(NULL, FALSE, FALSE, NULL);
	render_thread = utility::CreateThread(NULL, 0, this, &awkawk::render_thread_proc, static_cast<void*>(0), "Render Thread", 0, 0);
	schedule_render();
	return get_ui()->pump_messages();
}

void awkawk::stop_rendering()
{
	::SetEvent(cancel_render);
	::WaitForSingleObject(render_thread, INFINITE);
	::CloseHandle(cancel_render);
	::CloseHandle(render_thread);
	::CloseHandle(render_timer);
}

bool awkawk::needs_display_change() const
{
	D3DDEVICE_CREATION_PARAMETERS parameters;
	scene_device->GetCreationParameters(&parameters);
	HMONITOR device_monitor(d3d9.get_d3d9()->GetAdapterMonitor(parameters.AdapterOrdinal));
	HMONITOR window_monitor(::MonitorFromWindow(get_ui()->get_window(), MONITOR_DEFAULTTONEAREST));
	return device_monitor != window_monitor;
}

void awkawk::reset()
{
	switch(scene_device->TestCooperativeLevel())
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
	case D3D_OK:
		return;
	}
}

void awkawk::render()
{
	try
	{
		// TODO This was locked for a reason
		LOCK(dshow->graph_cs);
		scene->set_volume(dshow->get_linear_volume());
		scene->set_playback_position(dshow->get_playback_position());

		IDirect3DSurface9Ptr back_buffer;
		scene_device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
		scene_device->SetRenderTarget(0, back_buffer);
		set_device_state();

		static D3DCOLOR col(D3DCOLOR_ARGB(0xff, 0, 0, 0));
		//col = col == D3DCOLOR_ARGB(0xff, 0xff, 0, 0) ? D3DCOLOR_ARGB(0xff, 0, 0xff, 0)
		//    : col == D3DCOLOR_ARGB(0xff, 0, 0xff, 0) ? D3DCOLOR_ARGB(0xff, 0, 0, 0xff)
		//                                             : D3DCOLOR_ARGB(0xff, 0xff, 0, 0);
		//col = D3DCOLOR_ARGB(0xff, 0, 0xff, 0);
		FAIL_THROW(scene_device->Clear(0L, NULL, D3DCLEAR_TARGET, col, 1.0f, 0));
		{
			FAIL_THROW(scene_device->BeginScene());
			ON_BLOCK_EXIT_OBJ(*scene_device, &IDirect3DDevice9::EndScene);
			D3DXMATRIX ortho2D;
			D3DXMatrixOrthoLH(&ortho2D, static_cast<float>(window_size.cx), static_cast<float>(window_size.cy), 0.0f, 1.0f);
			FAIL_THROW(scene_device->SetTransform(D3DTS_PROJECTION, &ortho2D));
			boost::shared_ptr<utility::critical_section> stream_cs;
			std::auto_ptr<utility::critical_section::lock> l;
			if(dshow->get_has_video() && dshow->allocator->rendering(dshow->get_user_id()))
			{
				stream_cs = dshow->allocator->get_cs(dshow->get_user_id());
				l.reset(new utility::critical_section::lock(*stream_cs));
				scene->set_video_texture(dshow->allocator->get_video_texture(dshow->get_user_id()));
			}
			else
			{
				scene->set_video_texture(NULL);
			}
			scene->render();
			overlay->render();
		}
		FAIL_THROW(scene_device->Present(NULL, NULL, NULL, NULL));
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
}

DWORD awkawk::render_thread_proc(void*)
{
	try
	{
		//get_ui()->send_message(player_window::create_d3d_msg, 0, 0);
		create_d3d();
		//get_ui()->send_message(player_window::create_device_msg, 0, 0);
		create_device();
		HANDLE evts[] = { render_event, render_timer, cancel_render };
		bool continue_rendering(true);
		while(continue_rendering)
		{
			switch(::WaitForMultipleObjects(ARRAY_SIZE(evts), evts, FALSE, INFINITE))
			{
			case WAIT_OBJECT_0:
			case WAIT_OBJECT_0 + 1:
				schedule_render();
				try
				{
					if(needs_display_change())
					{
						//get_ui()->post_message(player_window::reset_device_msg, 0, 0);
						reset_device();
					}
					//get_ui()->post_message(player_window::reset_msg, 0, 0);
					reset();
					//get_ui()->post_message(player_window::render_msg, 0, 0);
					render();
				}
				catch(_com_error& ce)
				{
					derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
				}
				break;
			case WAIT_OBJECT_0 + 2:
			default:
				continue_rendering = false;
				break;
			}
		}
		//get_ui()->post_message(player_window::destroy_device_msg, 0, 0);
		destroy_device();
		//get_ui()->post_message(player_window::destroy_d3d_msg, 0, 0);
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

size_t awkawk::do_load()
{
	scene->set_filename(plist->get_file_name());
	dshow->send_message(player_direct_show::load);
	return 0;
}

size_t awkawk::do_stop()
{
	get_ui()->set_on_top(false);
	dshow->send_message(player_direct_show::stop);
	return 0;
}

size_t awkawk::do_pause()
{
	dshow->send_message(player_direct_show::pause);
	return 0;
}

size_t awkawk::do_resume()
{
	dshow->send_message(player_direct_show::pause);
	return 0;
}

size_t awkawk::do_play()
{
	get_ui()->set_on_top(true);
	dshow->send_message(player_direct_show::play);
	return 0;
}

size_t awkawk::do_unload()
{
	scene->set_filename(L"");
	dshow->send_message(player_direct_show::unload);
	return 0;
}

size_t awkawk::do_transition()
{
	if(plist->empty())
	{
		return 0;
	}
	awkawk_state initial_state(get_current_state());
	if(unloaded != initial_state)
	{
		if(stopped != initial_state)
		{
			process_message(stop);
		}
		process_message(unload);
	}
	plist->do_transition();

	if(!plist->after_end())
	{
		process_message(load);
		if(initial_state == playing)
		{
			process_message(play);
			return 2;
		}
		return 1;
	}
	return 0;
}

size_t awkawk::do_previous()
{
	if(plist->empty())
	{
		return 0;
	}
	awkawk_state initial_state(get_current_state());
	if(unloaded != initial_state)
	{
		if(stopped != initial_state)
		{
			process_message(stop);
		}
		process_message(unload);
	}
	plist->do_previous();

	if(!plist->after_end())
	{
		process_message(load);
		if(initial_state == playing)
		{
			process_message(play);
			return 2;
		}
		return 1;
	}
	return 0;
}

size_t awkawk::do_next()
{
	if(plist->empty())
	{
		return 0;
	}
	awkawk_state initial_state(get_current_state());
	if(unloaded != initial_state)
	{
		if(stopped != initial_state)
		{
			process_message(stop);
		}
		process_message(unload);
	}
	plist->do_next();

	if(!plist->after_end())
	{
		process_message(load);
		if(initial_state == playing)
		{
			process_message(play);
			return 2;
		}
		return 1;
	}
	return 0;
}

size_t awkawk::do_rwnd()
{
	dshow->send_message(player_direct_show::rwnd);
	return 0;
}

size_t awkawk::do_ffwd()
{
	dshow->send_message(player_direct_show::ffwd);
	return 0;
}

float awkawk::get_volume() const
{
	return dshow->get_volume();
}

void awkawk::set_linear_volume(float vol)
{
	dshow->set_linear_volume(vol);
}

void awkawk::set_playback_position(float pos)
{
	dshow->set_playback_position(pos);
}

float awkawk::get_play_time() const
{
	return dshow->get_play_time();
}

std::vector<CAdapt<IBaseFilterPtr> > awkawk::get_filters() const
{
	return dshow->get_filters();
}
