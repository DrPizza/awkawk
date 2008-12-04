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

#pragma once

#ifndef PLAYER_DIRECT_SHOW__H
#define PLAYER_DIRECT_SHOW__H

#include "util.h"

#include "state_machine.h"
#include "awkawk.h"
#include "allocator.h"

_COM_SMARTPTR_TYPEDEF(IFilterGraph, __uuidof(IFilterGraph));
_COM_SMARTPTR_TYPEDEF(IFilterGraph2, __uuidof(IFilterGraph2));
_COM_SMARTPTR_TYPEDEF(IBaseFilter, __uuidof(IBaseFilter));
_COM_SMARTPTR_TYPEDEF(IMediaControl, __uuidof(IMediaControl));
_COM_SMARTPTR_TYPEDEF(IVMRSurfaceAllocator9, __uuidof(IVMRSurfaceAllocator9));
_COM_SMARTPTR_TYPEDEF(IVMRFilterConfig9, __uuidof(IVMRFilterConfig9));
_COM_SMARTPTR_TYPEDEF(IVMRMixerControl9, __uuidof(IVMRMixerControl9));
_COM_SMARTPTR_TYPEDEF(IRunningObjectTable, __uuidof(IRunningObjectTable));
_COM_SMARTPTR_TYPEDEF(IMoniker, __uuidof(IMoniker));
_COM_SMARTPTR_TYPEDEF(IMediaSeeking, __uuidof(IMediaSeeking));
_COM_SMARTPTR_TYPEDEF(IBasicAudio, __uuidof(IBasicAudio));
_COM_SMARTPTR_TYPEDEF(IBasicVideo2, __uuidof(IBasicVideo2));
_COM_SMARTPTR_TYPEDEF(IEnumFilters, __uuidof(IEnumFilters));
_COM_SMARTPTR_TYPEDEF(IEnumPins, __uuidof(IEnumPins));
_COM_SMARTPTR_TYPEDEF(IPin, __uuidof(IPin));
_COM_SMARTPTR_TYPEDEF(IMediaEventEx, __uuidof(IMediaEventEx));
_COM_SMARTPTR_TYPEDEF(IVMRDeinterlaceControl9, __uuidof(IVMRDeinterlaceControl9));

struct player_direct_show : direct3d_object, boost::noncopyable
{
	enum graph_state
	{
		unloaded,
		stopped,
		paused,
		playing,
		max_graph_states
	};

	enum graph_event
	{
		load,
		stop,
		pause,
		play,
		unload,
		rwnd,
		ffwd,
		max_graph_events
	};

	std::string event_name(graph_event evt) const
	{
		switch(evt)
		{
		case load:
			return"load";
		case stop:
			return"stop";
		case pause:
			return"pause";
		case play:
			return"play";
		case unload:
			return"unload";
		case rwnd:
			return"rwnd";
		case ffwd:
			return"ffwd";
		}
		return "(not an event)";
	}

	typedef transition<player_direct_show, graph_state> transition_type;
	typedef event_handler<player_direct_show, graph_state, graph_event> event_handler_type;

	static const transition_type transitions[max_graph_states][max_graph_events];

	bool permitted(graph_event evt) const
	{
		return handler->permitted(evt);
	}

	player_direct_show(awkawk* player_) : player(player_),
	                                      handler(new event_handler_type(this)),
	                                      current_state(unloaded),
	                                      user_id(0xabcd),
	                                      has_video(false),
	                                      allocator(NULL),
	                                      media_event(NULL),
	                                      cancel_media_event(NULL),
	                                      media_event_thread(NULL)
	{
		LOCK(graph_cs);
		FAIL_THROW(graph.CreateInstance(CLSID_FilterGraph));
		FAIL_THROW(graph->QueryInterface(&events));
		HANDLE evt(0);
		FAIL_THROW(events->GetEventHandle(reinterpret_cast<OAEVENT*>(&evt)));
		::DuplicateHandle(::GetCurrentProcess(), evt, ::GetCurrentProcess(), &media_event, 0, FALSE, DUPLICATE_SAME_ACCESS);
		cancel_media_event = ::CreateEventW(NULL, TRUE, FALSE, NULL);
		media_event_thread = utility::CreateThread(NULL, 0, this, &player_direct_show::media_event_thread_proc, static_cast<void*>(0), "Media Event thread", 0, 0);
	}

	~player_direct_show()
	{
		if(get_graph_state() != unloaded)
		{
			if(get_graph_state() != stopped)
			{
				player->send_message(awkawk::stop);
			}
			player->send_message(awkawk::unload);
		}
		{
			::SetEvent(cancel_media_event);
			::WaitForSingleObject(media_event_thread, INFINITE);
			::CloseHandle(media_event_thread);
			::CloseHandle(cancel_media_event);
			::CloseHandle(media_event);
		}
		LOCK(graph_cs);
		events = NULL;
		graph = NULL;
	}

	graph_state send_message(graph_event evt)
	{
		return handler->send_message(evt);
	}

	void post_message(graph_event evt)
	{
		return handler->post_message(evt);
	}

	graph_state process_message(DWORD message)
	{
		return handler->process_message(message);
	}

	graph_state get_graph_state() const { return current_state; }
	graph_state get_current_state() const { return current_state; }
	void set_current_state(graph_state st) { current_state = st; }

	std::string state_name(graph_state st) const
	{
		switch(st)
		{
		case unloaded:
			return "unloaded";
		case stopped:
			return "stopped";
		case paused:
			return "paused";
		case playing:
			return "playing";
		}
		return "error";
	}

	OAFilterState get_movie_state() const
	{
		OAFilterState state;
		FAIL_THROW(media_control->GetState(0, &state));
		return state;
	}

	std::vector<CAdapt<IBaseFilterPtr> > get_filters() const
	{
		std::vector<CAdapt<IBaseFilterPtr> > rv;
		IEnumFiltersPtr filtEn;
		graph->EnumFilters(&filtEn);
		for(IBaseFilterPtr flt; S_OK == filtEn->Next(1, &flt, NULL);)
		{
			rv.push_back(flt);
		}
		return rv;
	}

	void set_playback_position(float percentage)
	{
		if(get_graph_state() != unloaded)
		{
			LOCK(graph_cs);
			LONGLONG end(0);
			seeking->GetStopPosition(&end);
			LONGLONG position(static_cast<LONGLONG>(percentage * static_cast<float>(end) / 100.0));
			seeking->SetPositions(&position, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
		}
	}

	float get_play_time() const
	{
		static const REFERENCE_TIME ticks_in_one_second(10000000);
		try
		{
			LOCK(graph_cs);
			if(get_graph_state() != unloaded)
			{
				REFERENCE_TIME current(0);
				FAIL_THROW(seeking->GetCurrentPosition(&current));
				return static_cast<float>(current) / ticks_in_one_second;
			}
		}
		catch(_com_error& ce)
		{
			switch(ce.Error())
			{
			case E_NOTIMPL:
			case E_FAIL:
				return -1.0f;
			default:
				throw;
			}
		}
		return -1.0f;
	}

	float get_total_time() const
	{
		static const REFERENCE_TIME ticks_in_one_second(10000000);
		try
		{
			LOCK(graph_cs);
			if(get_graph_state() != unloaded)
			{
				REFERENCE_TIME duration(0);
				FAIL_THROW(seeking->GetDuration(&duration));
				return static_cast<float>(duration) / ticks_in_one_second;
			}
		}
		catch(_com_error& ce)
		{
			switch(ce.Error())
			{
			case E_NOTIMPL:
			case E_FAIL:
				return -1.0f;
			default:
				throw;
			}
		}
		return -1.0f;
	}

	float get_playback_position() const
	{
		try
		{
			LOCK(graph_cs);
			if(get_graph_state() != unloaded)
			{
				LONGLONG current(0), duration(0);
				FAIL_THROW(seeking->GetCurrentPosition(&current));
				FAIL_THROW(seeking->GetDuration(&duration));
				return 100.0f * static_cast<float>(current) / static_cast<float>(duration);
			}
		}
		catch(_com_error& ce)
		{
			switch(ce.Error())
			{
			case E_NOTIMPL:
			case E_FAIL:
				return -1.0f;
			default:
				throw;
			}
		}
		return -1.0f;
	}

	// in dB, from -100.0 to 0.0
	float get_volume() const
	{
		if(get_graph_state() != unloaded)
		{
			LOCK(graph_cs);
			LONG volume(0);
			audio->get_Volume(&volume);
			return (static_cast<float>(volume) / 100.0f);
		}
		return -std::numeric_limits<float>::infinity();
	}

	void set_volume(float vol)
	{
		if(get_graph_state() != unloaded)
		{
			LOCK(graph_cs);
			LONG volume(static_cast<LONG>(vol * 100.0f));
			audio->put_Volume(volume);
		}
	}

	// linearized such that 0 dB = 1, -6 dB ~= 0.5, -inf dB = 0.0 (except directshow is annoying, and doesn't go to -inf, only -100)
	float get_linear_volume() const
	{
		float raw_volume(get_volume());
		if(raw_volume <= -100.0)
		{
			return 0;
		}
		return std::pow(10.0f, get_volume() / 20.0f);
	}

	void set_linear_volume(float vol)
	{
		vol = clamp(vol, 0.0f, 1.0f);
		vol = std::log10(vol) * 20.0f;
		vol = clamp(vol, -100.0f, 0.0f);
		set_volume(vol);
	}

	REFERENCE_TIME get_average_frame_time(IFilterGraphPtr grph) const;
	SIZE get_video_size() const;

	DWORD_PTR get_user_id() const
	{
		return user_id;
	}

	bool get_has_video() const
	{
		return has_video;
	}

	// TODO make allocator non-public
	allocator_presenter* allocator;
	// TODO make graph_cs non-public
	mutable utility::critical_section graph_cs;

protected:
	// create D3DPOOL_MANAGED resources
	virtual HRESULT do_on_device_created(IDirect3DDevice9Ptr new_device)
	{
		if(allocator)
		{
			return allocator->on_device_created(new_device);
		}
		return S_OK;
	}

	// create D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_reset()
	{
		if(allocator)
		{
			return allocator->on_device_reset();
		}
		return S_OK;
	}

	// destroy D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_lost()
	{
		if(allocator)
		{
			return allocator->on_device_lost();
		}
		return S_OK;
	}

	// destroy D3DPOOL_MANAGED resources
	virtual void do_on_device_destroyed()
	{
		if(allocator)
		{
			allocator->on_device_destroyed();
		}
	}

private:
	void set_allocator_presenter(IBaseFilterPtr filter, HWND window);
	void create_graph();
	void destroy_graph();

	void register_graph(IUnknownPtr unknown);
	void unregister_graph();

	std::auto_ptr<event_handler_type> handler;

	size_t do_load();
	size_t do_stop();
	size_t do_pause();
	size_t do_resume();
	size_t do_play();
	size_t do_unload();
	size_t do_rwnd();
	size_t do_ffwd();

	awkawk* player;

	// graph state
	graph_state current_state;

	// ROT registration
	DWORD rot_key;

	HANDLE media_event;
	HANDLE cancel_media_event;
	HANDLE media_event_thread;
	DWORD media_event_thread_proc(void*);

	// DirectShow gubbins
	DWORD_PTR user_id;
	IFilterGraph2Ptr graph;
	IBaseFilterPtr vmr9;
	IMediaControlPtr media_control;
	IMediaSeekingPtr seeking;
	IBasicAudioPtr audio;
	IBasicVideo2Ptr video;
	IMediaEventExPtr events;

	bool has_video;

	IVMRSurfaceAllocator9Ptr vmr_surface_allocator;
};

#endif
