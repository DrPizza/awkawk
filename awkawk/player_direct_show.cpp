//  Copyright (C) 2008 Peter Bright
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

#include "player_direct_show.h"
#include "awkawk.h"

const player_direct_show::transition_type player_direct_show::transitions[player_direct_show::max_graph_states][player_direct_show::max_graph_events] =
{
/* state    | event         | handler                         | exit states */
/* ------------------- */ {
/* unloaded | load     */   { &player_direct_show::do_load      , player_direct_show::transition_type::state_array() << player_direct_show::stopped                                                                },
/*          | stop     */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               },
/*          | pause    */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               },
/*          | play     */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               },
/*          | unload   */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               },
/*          | rwnd     */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               },
/*          | ffwd     */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               }
                          },
                          {
/* stopped  | load     */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               },
/*          | stop     */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               },
/*          | pause    */   { &player_direct_show::do_play      , player_direct_show::transition_type::state_array() << player_direct_show::playing                                                                },
/*          | play     */   { &player_direct_show::do_play      , player_direct_show::transition_type::state_array() << player_direct_show::playing                                                                },
/*          | unload   */   { &player_direct_show::do_unload    , player_direct_show::transition_type::state_array() << player_direct_show::unloaded                                                               },
/*          | rwnd     */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               },
/*          | ffwd     */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               }
                          },
                          {
/* paused   | load     */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               },
/*          | stop     */   { &player_direct_show::do_stop      , player_direct_show::transition_type::state_array() << player_direct_show::stopped                                                                },
/*          | pause    */   { &player_direct_show::do_resume    , player_direct_show::transition_type::state_array() << player_direct_show::playing                                                                },
/*          | play     */   { &player_direct_show::do_resume    , player_direct_show::transition_type::state_array() << player_direct_show::playing                                                                },
/*          | unload   */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               },
/*          | rwnd     */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               },
/*          | ffwd     */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               }
                          },
                          {
/* playing  | load     */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               },
/*          | stop     */   { &player_direct_show::do_stop      , player_direct_show::transition_type::state_array() << player_direct_show::stopped                                                                },
/*          | pause    */   { &player_direct_show::do_pause     , player_direct_show::transition_type::state_array() << player_direct_show::paused                                                                 },
/*          | play     */   { &player_direct_show::do_pause     , player_direct_show::transition_type::state_array() << player_direct_show::paused                                                                 },
/*          | unload   */   { NULL                              , player_direct_show::transition_type::state_array()                                                                                               },
/*          | rwnd     */   { &player_direct_show::do_rwnd      , player_direct_show::transition_type::state_array() << player_direct_show::playing                                                                },
/*          | ffwd     */   { &player_direct_show::do_ffwd      , player_direct_show::transition_type::state_array() << player_direct_show::playing                                                                }
                          }
};

size_t player_direct_show::do_stop()
{
	dout << __FUNCSIG__ << std::endl;
	LOCK(graph_cs);
	OAFilterState movie_state;
	media_control->GetState(0, &movie_state);
	while(movie_state != State_Stopped)
	{
		FAIL_THROW(media_control->Stop());
		media_control->GetState(0, &movie_state);
	}
	return 0;
}

size_t player_direct_show::do_play()
{
	LOCK(graph_cs);
	FAIL_THROW(media_control->Run());
	return 0;
}

size_t player_direct_show::do_pause()
{
	dout << __FUNCSIG__ << std::endl;
	LOCK(graph_cs);
	OAFilterState movie_state;
	media_control->GetState(0, &movie_state);
	while(movie_state != State_Paused)
	{
		FAIL_THROW(media_control->Pause());
		media_control->GetState(0, &movie_state);
	}
	return 0;
}

size_t player_direct_show::do_resume()
{
	LOCK(graph_cs);
	FAIL_THROW(media_control->Run());
	return 0;
}

size_t player_direct_show::do_ffwd()
{
	LOCK(graph_cs);
	// TODO write ffwd code
	return 0;
}

size_t player_direct_show::do_rwnd()
{
	LOCK(graph_cs);
	// TODO write rwnd code
	return 0;
}

size_t player_direct_show::do_load()
{
	dout << __FUNCSIG__ << std::endl;
	LOCK(graph_cs);
	try
	{
		create_graph();
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
	return 0;
}

size_t player_direct_show::do_unload()
{
	LOCK(graph_cs);
	LONGLONG current(0);
	seeking->SetPositions(&current, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
	destroy_graph();
	has_video = false;
	return 0;
}

void player_direct_show::destroy_graph()
{
	LOCK(graph_cs);
	unregister_graph();

	IEnumFiltersPtr filtEn;
	graph->EnumFilters(&filtEn);
	for(IBaseFilterPtr flt; S_OK == filtEn->Next(1, &flt, NULL); graph->EnumFilters(&filtEn))
	{
		//FILTER_INFO fi = {0};
		//flt->QueryFilterInfo(&fi);
		//IFilterGraphPtr ptr(fi.pGraph, false);
		//wdout << L"Removing " << fi.achName << std::endl;
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

	player->set_render_fps(25);
	player->schedule_render();

}

void player_direct_show::set_allocator_presenter(IBaseFilterPtr filter, HWND window)
{
	LOCK(graph_cs);
	IVMRSurfaceAllocatorNotify9Ptr surface_allocator_notify;
	FAIL_THROW(filter->QueryInterface(&surface_allocator_notify));

	allocator = new allocator_presenter(player, player->get_scene_device());
	vmr_surface_allocator.Attach(allocator, true);

	FAIL_THROW(surface_allocator_notify->AdviseSurfaceAllocator(user_id, vmr_surface_allocator));
	FAIL_THROW(vmr_surface_allocator->AdviseNotify(surface_allocator_notify));
	FAIL_THROW(surface_allocator_notify->SetD3DDevice(player->get_scene_device(), ::MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY)));
}

REFERENCE_TIME player_direct_show::get_average_frame_time(IFilterGraphPtr grph) const
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
			ON_BLOCK_EXIT(&free_media_type, Loki::ByRef(mt));
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

SIZE player_direct_show::get_video_size() const
{
	IEnumPinsPtr pinEn;
	vmr9->EnumPins(&pinEn);
	for(IPinPtr pin; S_OK == pinEn->Next(1, &pin, NULL);)
	{
		AM_MEDIA_TYPE mt;
		pin->ConnectionMediaType(&mt);
		ON_BLOCK_EXIT(&free_media_type, Loki::ByRef(mt));
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
	derr << "Warning: no size detected" << std::endl;
	return player->get_video_dimensions();
}

void player_direct_show::create_graph()
{
	FAIL_THROW(vmr9.CreateInstance(CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER));
	IVMRFilterConfig9Ptr filter_config;
	FAIL_THROW(vmr9->QueryInterface(&filter_config));
	FAIL_THROW(filter_config->SetRenderingMode(VMR9Mode_Renderless));
	set_allocator_presenter(vmr9, player->get_ui()->get_window());
	// Note that we MUST use YUV mixing mode (or no mixing mode at all)
	// because if we don't, VMR9 can change the state of the D3D device
	// part-way through our render (and it doesn't tell us it's going to,
	// so we have no opportunity to protect ourselves from its stupidity)
	// when it performs colour space conversions.
	// Further, if we do not use mixing mode then we cannot use VMR deinterlacing.
	// It's all rather sad.
#define USE_MIXING_MODE
#define USE_YUV_MIXING_MODE
#ifdef USE_MIXING_MODE
	FAIL_THROW(filter_config->SetNumberOfStreams(1));
	IVMRMixerControl9Ptr mixer_control;
	FAIL_THROW(vmr9->QueryInterface(&mixer_control));
	DWORD mixing_prefs(0);
	FAIL_THROW(mixer_control->GetMixingPrefs(&mixing_prefs));
	mixing_prefs &= ~MixerPref9_RenderTargetMask;
#ifdef USE_YUV_MIXING_MODE
	mixing_prefs |= MixerPref9_RenderTargetYUV;
#endif
	mixing_prefs &= ~MixerPref9_DynamicMask;
	mixing_prefs &= ~MixerPref9_DecimateMask;
	mixing_prefs |= MixerPref9_NoDecimation;
	mixing_prefs |= MixerPref9_ARAdjustXorY;
	mixing_prefs &= ~MixerPref9_FilteringMask;
	mixing_prefs |= MixerPref9_PyramidalQuadFiltering;
	FAIL_THROW(mixer_control->SetMixingPrefs(mixing_prefs));
#endif

	FAIL_THROW(graph->AddFilter(vmr9, L"Video Mixing Renderer 9"));

	_bstr_t current_item(player->get_file_name().c_str());
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
		ON_BLOCK_EXIT(&free_media_type, Loki::ByRef(mt));
		has_video = has_video || (mt.formattype != FORMAT_None) && (mt.formattype != GUID_NULL);
	}

	if(has_video)
	{
		player->set_video_dimensions(get_video_size());
	}

	REFERENCE_TIME average_frame_time(get_average_frame_time(graph));

	float frame_rate(0 == average_frame_time ? 25 : 10000000 / static_cast<float>(average_frame_time));
	player->set_render_fps(static_cast<unsigned int>(frame_rate));
}

void player_direct_show::register_graph(IUnknownPtr unknown)
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

void player_direct_show::unregister_graph(void)
{
	IRunningObjectTablePtr rot;
	FAIL_THROW(::GetRunningObjectTable(0, &rot));
	rot->Revoke(rot_key);
}

DWORD player_direct_show::media_event_thread_proc(void*)
{
	try
	{
		HANDLE evts[] = { media_event, cancel_media_event };
		while(::WaitForMultipleObjects(ARRAY_SIZE(evts), evts, FALSE, INFINITE) == WAIT_OBJECT_0)
		{
			long event_code;
			LONG_PTR param1;
			LONG_PTR param2;
			FAIL_THROW(events->GetEvent(&event_code, &param1, &param2, INFINITE));
			ON_BLOCK_EXIT_OBJ(*events, &IMediaEvent::FreeEventParams, event_code, param1, param2);
			switch(event_code)
			{
			case EC_COMPLETE:
				{
					dout << "EC_COMPLETE" << std::endl;
					player->post_message(awkawk::transitioning);
				}
				break;
			// ( HRESULT, void ) : defaulted (special)
			case EC_USERABORT: dout << "EC_USERABORT" << std::endl; break;
			// ( void, void ) : application
			case EC_ERRORABORT: dout << "EC_ERRORABORT" << std::endl; break;
			// ( HRESULT, void ) : application
			case EC_TIME: dout << "EC_TIME" << std::endl; break;
			// ( DWORD, DWORD ) : application
			case EC_REPAINT: dout << "EC_REPAINT" << std::endl; break;
			// ( IPin * (could be NULL), void ) : defaulted
			case EC_STREAM_ERROR_STOPPED: dout << "EC_STREAM_ERROR_STOPPED" << std::endl; break;
			case EC_STREAM_ERROR_STILLPLAYING: dout << "EC_STREAM_ERROR_STILLPLAYING" << std::endl; break;
			// ( HRESULT, DWORD ) : application
			case EC_ERROR_STILLPLAYING: dout << "EC_ERROR_STILLPLAYING" << std::endl; break;
			// ( HRESULT, void ) : application
			case EC_PALETTE_CHANGED: dout << "EC_PALETTE_CHANGED" << std::endl; break;
			// ( void, void ) : application
			case EC_VIDEO_SIZE_CHANGED:
				// the documentation says that this message doesn't get sent in renderless mode.
				// at least on Vista, the documentation is full of shit.
				{
					dout << "EC_VIDEO_SIZE_CHANGED: " << std::dec << LOWORD(param1) << " x " << HIWORD(param1) << std::endl;
				}
				break;
			// ( DWORD, void ) : application
			// LOWORD of the DWORD is the new width, HIWORD is the new height.
			case EC_QUALITY_CHANGE: dout << "EC_QUALITY_CHANGE" << std::endl; break;
			// ( void, void ) : application
			case EC_SHUTTING_DOWN: dout << "EC_SHUTTING_DOWN" << std::endl; break;
			// ( void, void ) : internal
			case EC_CLOCK_CHANGED: dout << "EC_CLOCK_CHANGED" << std::endl; break;
			// ( void, void ) : application
			case EC_PAUSED: dout << "EC_PAUSED" << std::endl; break;
			// ( HRESULT, void ) : application
			case EC_OPENING_FILE: dout << "EC_OPENING_FILE" << std::endl; break;
			case EC_BUFFERING_DATA: dout << "EC_BUFFERING_DATA" << std::endl; break;
			// ( BOOL, void ) : application
			case EC_FULLSCREEN_LOST: dout << "EC_FULLSCREEN_LOST" << std::endl; break;
			// ( void, IBaseFilter * ) : application
			case EC_ACTIVATE: dout << "EC_ACTIVATE" << std::endl; break;
			// ( BOOL, IBaseFilter * ) : internal
			case EC_NEED_RESTART: dout << "EC_NEED_RESTART" << std::endl; break;
			// ( void, void ) : defaulted
			case EC_WINDOW_DESTROYED: dout << "EC_WINDOW_DESTROYED" << std::endl; break;
			// ( IBaseFilter *, void ) : internal
			case EC_DISPLAY_CHANGED: dout << "EC_DISPLAY_CHANGED" << std::endl; break;
			// ( IPin *, void ) : internal
			case EC_STARVATION: dout << "EC_STARVATION" << std::endl; break;
			// ( void, void ) : defaulted
			case EC_OLE_EVENT: dout << "EC_OLE_EVENT" << std::endl; break;
			// ( BSTR, BSTR ) : application
			case EC_NOTIFY_WINDOW: dout << "EC_NOTIFY_WINDOW" << std::endl; break;
			// ( HWND, void ) : internal
			case EC_STREAM_CONTROL_STOPPED: dout << "EC_STREAM_CONTROL_STOPPED" << std::endl; break;
			// ( IPin * pSender, DWORD dwCookie )
			case EC_STREAM_CONTROL_STARTED: dout << "EC_STREAM_CONTROL_STARTED" << std::endl; break;
			// ( IPin * pSender, DWORD dwCookie )
			case EC_END_OF_SEGMENT: dout << "EC_END_OF_SEGMENT" << std::endl; break;
			// ( const REFERENCE_TIME *pStreamTimeAtEndOfSegment, DWORD dwSegmentNumber )
			case EC_SEGMENT_STARTED: dout << "EC_SEGMENT_STARTED" << std::endl; break;
			// ( const REFERENCE_TIME *pStreamTimeAtStartOfSegment, DWORD dwSegmentNumber)
			case EC_LENGTH_CHANGED: dout << "EC_LENGTH_CHANGED" << std::endl; break;
			// (void, void)
			case EC_DEVICE_LOST: dout << "EC_DEVICE_LOST" << std::endl; break;
			// (IUnknown, 0)
			case EC_STEP_COMPLETE: dout << "EC_STEP_COMPLETE" << std::endl; break;
			// (BOOL bCacelled, void)
			case EC_TIMECODE_AVAILABLE: dout << "EC_TIMECODE_AVAILABLE" << std::endl; break;
			// Param1 has a pointer to the sending object
			// Param2 has the device ID of the sending object
			case EC_EXTDEVICE_MODE_CHANGE: dout << "EC_EXTDEVICE_MODE_CHANGE" << std::endl; break;
			// Param1 has the new mode
			// Param2 has the device ID of the sending object
			case EC_STATE_CHANGE: dout << "EC_STATE_CHANGE" << std::endl; break;
			// ( FILTER_STATE, BOOL bInternal)
			case EC_GRAPH_CHANGED: dout << "EC_GRAPH_CHANGED" << std::endl; break;
			// Sent by filter to notify interesting graph changes
			case EC_CLOCK_UNSET: dout << "EC_CLOCK_UNSET" << std::endl; break;
			// ( void, void ) : application
			case EC_VMR_RENDERDEVICE_SET: dout << "EC_VMR_RENDERDEVICE_SET" << std::endl; break;
			// (Render_Device type, void)
			case EC_VMR_SURFACE_FLIPPED: dout << "EC_VMR_SURFACE_FLIPPED" << std::endl; break;
			// (hr - Flip return code, void)
			case EC_VMR_RECONNECTION_FAILED: dout << "EC_VMR_RECONNECTION_FAILED" << std::endl; break;
			// (hr - ReceiveConnection return code, void)
			case EC_PREPROCESS_COMPLETE: dout << "EC_PREPROCESS_COMPLETE" << std::endl; break;
			// Param1 = 0, Param2 = IBaseFilter ptr of sending filter
			case EC_CODECAPI_EVENT: dout << "EC_CODECAPI_EVENT" << std::endl; break;
			// Param1 = UserDataPointer, Param2 = VOID* Data
			case EC_WMT_INDEX_EVENT: dout << "EC_WMT_INDEX_EVENT" << std::endl; break;
			// lParam1 is one of the enum WMT_STATUS messages listed below, sent by the WindowsMedia SDK
			// lParam2 is specific to the lParam event
			case EC_WMT_EVENT: dout << "EC_WMT_EVENT" << std::endl; break;
			// lParam1 is one of the enum WMT_STATUS messages listed below, sent by the WindowsMedia SDK
			// lParam2 is a pointer an AM_WMT_EVENT_DATA structure where,
			//                          hrStatus is the status code sent by the wmsdk
			//                          pData is specific to the lParam1 event
			case EC_BUILT: dout << "EC_BUILT" << std::endl; break;
			// Sent to notify transition from unbuilt to built state
			case EC_UNBUILT: dout << "EC_UNBUILT" << std::endl; break;
			// Sent to notify transtion from built to unbuilt state
			case EC_DVD_DOMAIN_CHANGE: dout << "EC_DVD_DOMAIN_CHANGE" << std::endl; break;
			// Parameters: ( DWORD, void ) 
			case EC_DVD_TITLE_CHANGE: dout << "EC_DVD_TITLE_CHANGE" << std::endl; break;
			// Parameters: ( DWORD, void ) 
			case EC_DVD_CHAPTER_START: dout << "EC_DVD_CHAPTER_START" << std::endl; break;
			// Parameters: ( DWORD, void ) 
			case EC_DVD_AUDIO_STREAM_CHANGE: dout << "EC_DVD_AUDIO_STREAM_CHANGE" << std::endl; break;
			// Parameters: ( DWORD, void ) 
			case EC_DVD_SUBPICTURE_STREAM_CHANGE: dout << "EC_DVD_SUBPICTURE_STREAM_CHANGE" << std::endl; break;
			// Parameters: ( DWORD, BOOL ) 
			case EC_DVD_ANGLE_CHANGE: dout << "EC_DVD_ANGLE_CHANGE" << std::endl; break;
			// Parameters: ( DWORD, DWORD ) 
			case EC_DVD_BUTTON_CHANGE: dout << "EC_DVD_BUTTON_CHANGE" << std::endl; break;
			// Parameters: ( DWORD, DWORD ) 
			case EC_DVD_VALID_UOPS_CHANGE: dout << "EC_DVD_VALID_UOPS_CHANGE" << std::endl; break;
			// Parameters: ( DWORD, void ) 
			case EC_DVD_STILL_ON: dout << "EC_DVD_STILL_ON" << std::endl; break;
			// Parameters: ( BOOL, DWORD ) 
			case EC_DVD_STILL_OFF: dout << "EC_DVD_STILL_OFF" << std::endl; break;
			// Parameters: ( void, void ) 
			case EC_DVD_CURRENT_TIME: dout << "EC_DVD_CURRENT_TIME" << std::endl; break;
			// Parameters: ( DWORD, BOOL ) 
			case EC_DVD_ERROR: dout << "EC_DVD_ERROR" << std::endl; break;
			// Parameters: ( DWORD, void) 
			case EC_DVD_WARNING: dout << "EC_DVD_WARNING" << std::endl; break;
			// Parameters: ( DWORD, DWORD) 
			case EC_DVD_CHAPTER_AUTOSTOP: dout << "EC_DVD_CHAPTER_AUTOSTOP" << std::endl; break;
			// Parameters: (BOOL, void)
			case EC_DVD_NO_FP_PGC: dout << "EC_DVD_NO_FP_PGC" << std::endl; break;
			//  Parameters : (void, void)
			case EC_DVD_PLAYBACK_RATE_CHANGE: dout << "EC_DVD_PLAYBACK_RATE_CHANGE" << std::endl; break;
			//  Parameters : (LONG, void)
			case EC_DVD_PARENTAL_LEVEL_CHANGE: dout << "EC_DVD_PARENTAL_LEVEL_CHANGE" << std::endl; break;
			//  Parameters : (LONG, void)
			case EC_DVD_PLAYBACK_STOPPED: dout << "EC_DVD_PLAYBACK_STOPPED" << std::endl; break;
			//  Parameters : (DWORD, void)
			case EC_DVD_ANGLES_AVAILABLE: dout << "EC_DVD_ANGLES_AVAILABLE" << std::endl; break;
			//  Parameters : (BOOL, void)
			case EC_DVD_PLAYPERIOD_AUTOSTOP: dout << "EC_DVD_PLAYPERIOD_AUTOSTOP" << std::endl; break;
			// Parameters: (void, void)
			case EC_DVD_BUTTON_AUTO_ACTIVATED: dout << "EC_DVD_BUTTON_AUTO_ACTIVATED" << std::endl; break;
			// Parameters: (DWORD button, void)
			case EC_DVD_CMD_START: dout << "EC_DVD_CMD_START" << std::endl; break;
			// Parameters: (CmdID, HRESULT)
			case EC_DVD_CMD_END: dout << "EC_DVD_CMD_END" << std::endl; break;
			// Parameters: (CmdID, HRESULT)
			case EC_DVD_DISC_EJECTED: dout << "EC_DVD_DISC_EJECTED" << std::endl; break;
			// Parameters: none
			case EC_DVD_DISC_INSERTED: dout << "EC_DVD_DISC_INSERTED" << std::endl; break;
			// Parameters: none
			case EC_DVD_CURRENT_HMSF_TIME: dout << "EC_DVD_CURRENT_HMSF_TIME" << std::endl; break;
			// Parameters: ( ULONG, ULONG ) 
			case EC_DVD_KARAOKE_MODE: dout << "EC_DVD_KARAOKE_MODE" << std::endl; break;
			// Parameters: ( BOOL, reserved ) 
			default:
				dout << "unknown event: " << event_code << std::endl;
			}
		}
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

#if 0

#define IsInterlaced(x) ((x) & AMINTERLACE_IsInterlaced)
#define IsSingleField(x) ((x) & AMINTERLACE_1FieldPerSample)
#define IsField1First(x) ((x) & AMINTERLACE_Field1First)

VMR9_SampleFormat ConvertInterlaceFlags(DWORD dwInterlaceFlags)
{
	if(IsInterlaced(dwInterlaceFlags))
	{
		if(IsSingleField(dwInterlaceFlags))
		{
			if(IsField1First(dwInterlaceFlags))
			{
				return VMR9_SampleFieldSingleEven;
			}
			else
			{
				return VMR9_SampleFieldSingleOdd;
			}
		}
		else
		{
			if(IsField1First(dwInterlaceFlags))
			{
				return VMR9_SampleFieldInterleavedEvenFirst;
			}
			else
			{
				return VMR9_SampleFieldInterleavedOddFirst;
			}
		}
	}
	else
	{
		return VMR9_SampleProgressiveFrame;  // Not interlaced.
	}
}


		if(mt.majortype == MEDIATYPE_Video && mt.formattype == FORMAT_VideoInfo2)
		{
			VIDEOINFOHEADER2* info(reinterpret_cast<VIDEOINFOHEADER2*>(mt.pbFormat));
			switch(ConvertInterlaceFlags(info->dwInterlaceFlags))
			{
			case VMR9_SampleProgressiveFrame:
				dout << "Progressive" << std::endl;
				break;
			case VMR9_SampleFieldInterleavedEvenFirst:
			case VMR9_SampleFieldInterleavedOddFirst:
			case VMR9_SampleFieldSingleEven:
			case VMR9_SampleFieldSingleOdd:
				dout << "Interlaced" << std::endl;
				{
					BITMAPINFOHEADER* bmi(&(info->bmiHeader));
					VMR9VideoDesc desc = { sizeof(VMR9VideoDesc) };
					desc.dwSampleWidth = bmi->biWidth;
					desc.dwSampleHeight = std::abs(bmi->biHeight);
					desc.SampleFormat = ConvertInterlaceFlags(info->dwInterlaceFlags);
					desc.InputSampleFreq.dwNumerator = 10000000;
					desc.InputSampleFreq.dwDenominator = info->AvgTimePerFrame;
					desc.OutputFrameFreq.dwNumerator = desc.InputSampleFreq.dwNumerator * 2;
					desc.OutputFrameFreq.dwDenominator = desc.InputSampleFreq.dwDenominator;
					desc.dwFourCC = bmi->biCompression;
					DWORD num_modes(0);
					IVMRDeinterlaceControl9Ptr di_control;
					vmr9->QueryInterface(&di_control);
					di_control->GetNumberOfDeinterlaceModes(&desc, &num_modes, NULL);
					boost::scoped_array<GUID> guids(new GUID[num_modes]);
					dout << "num_modes: " << num_modes << std::endl;
					di_control->GetNumberOfDeinterlaceModes(&desc, &num_modes, guids.get());
					for(DWORD i(0); i < num_modes; ++i)
					{
						VMR9DeinterlaceCaps caps = { sizeof(VMR9DeinterlaceCaps) };
						di_control->GetDeinterlaceModeCaps(&(guids[i]), &desc, &caps);
						dout << guids[i] << std::endl;
					}
				}
				break;
			}
		}
#endif
