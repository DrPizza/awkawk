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

	const transition_type* get_transition(graph_event evt)
	{
		return &(transitions[get_current_state()][evt]);
	}

	bool permitted(graph_event evt) const
	{
		return handler->permitted(evt);
	}
	typedef message_pump<player_direct_show> message_pump_type;

	enum graph_message
	{
		msg_initialize_filter_graph,
		msg_create_graph,
		msg_destroy_graph,
		msg_destroy_filter_graph,
		msg_load,
		msg_unload,
		msg_ffwd,
		msg_rwnd,
		msg_pause,
		msg_play,
		msg_resume,
		msg_stop,
		msg_get_filters,
		msg_get_volume,
		msg_set_volume,
		msg_get_movie_state,
		msg_get_play_time,
		msg_get_playback_position,
		msg_set_playback_position,
		msg_get_total_time,
	};

	const message_pump_type::message_callback_type get_callback(ULONG_PTR message) const
	{
		switch(message)
		{
		case msg_initialize_filter_graph:
			return &player_direct_show::do_msg_initialize_filter_graph;
		case msg_create_graph:
			return &player_direct_show::do_msg_create_graph;
		case msg_destroy_graph:
			return &player_direct_show::do_msg_destroy_graph;
		case msg_destroy_filter_graph:
			return &player_direct_show::do_msg_destroy_filter_graph;
		case msg_load:
			return &player_direct_show::do_msg_load;
		case msg_unload:
			return &player_direct_show::do_msg_unload;
		case msg_ffwd:
			return &player_direct_show::do_msg_ffwd;
		case msg_rwnd:
			return &player_direct_show::do_msg_rwnd;
		case msg_pause:
			return &player_direct_show::do_msg_pause;
		case msg_play:
			return &player_direct_show::do_msg_play;
		case msg_resume:
			return &player_direct_show::do_msg_resume;
		case msg_stop:
			return &player_direct_show::do_msg_stop;
		case msg_get_filters:
			return &player_direct_show::do_msg_get_filters;
		case msg_get_volume:
			return &player_direct_show::do_msg_get_volume;
		case msg_set_volume:
			return &player_direct_show::do_msg_set_volume;
		case msg_get_movie_state:
			return &player_direct_show::do_msg_get_movie_state;
		case msg_get_play_time:
			return &player_direct_show::do_msg_get_play_time;
		case msg_get_playback_position:
			return &player_direct_show::do_msg_get_playback_position;
		case msg_set_playback_position:
			return &player_direct_show::do_msg_set_playback_position;
		case msg_get_total_time:
			return &player_direct_show::do_msg_get_total_time;
		}
		return NULL;
	}

	player_direct_show(awkawk* player_) : player(player_),
	                                      pump(new message_pump_type(this)),
	                                      handler(new event_handler_type(this)),
	                                      current_state(unloaded),
	                                      user_id(0xabcd),
	                                      has_video(false),
	                                      cs("graph"),
	                                      allocator(NULL),
	                                      media_event(NULL),
	                                      cancel_media_event(NULL),
	                                      media_event_thread(NULL)
	{
		pump->send_message(msg_initialize_filter_graph, NULL);
	}

	~player_direct_show()
	{
		pump->send_message(msg_destroy_filter_graph, NULL);
	}

	graph_state send_event(graph_event evt)
	{
		return handler->send_event(evt);
	}

	void post_event(graph_event evt)
	{
		return handler->post_event(evt);
	}

	graph_state get_graph_state() const { return current_state; }
	graph_state get_current_state() const { return current_state; }
	void set_current_state(graph_state st) { current_state = st; }

	std::string state_name(graph_state st) const
	{
		switch(st)
		{
		case unloaded:
			return "player_direct_show::unloaded";
		case stopped:
			return "player_direct_show::stopped";
		case paused:
			return "player_direct_show::paused";
		case playing:
			return "player_direct_show::playing";
		}
		::DebugBreak();
		return "player_direct_show::error";
	}

	OAFilterState get_movie_state() const
	{
		OAFilterState state;
		pump->execute_directly(msg_get_movie_state, &state);
		return state;
	}

	std::vector<ATL::CAdapt<IBaseFilterPtr> > get_filters() const
	{
		std::vector<ATL::CAdapt<IBaseFilterPtr> > result;
		pump->execute_directly(msg_get_filters, &result);
		return result;
	}

	float get_play_time() const
	{
		float result;
		pump->execute_directly(msg_get_play_time, &result);
		return result;
	}

	float get_total_time() const
	{
		float result;
		pump->execute_directly(msg_get_total_time, &result);
		return result;
	}

	float get_playback_position() const
	{
		float result;
		pump->execute_directly(msg_get_playback_position, &result);
		return result;
	}

	void set_playback_position(float percentage)
	{
		std::auto_ptr<float> pcnt(new float(percentage));
		pump->post_message_callback(msg_set_playback_position, pcnt.release(), this, &player_direct_show::async_callback, NULL);
	}

	// in dB, from -100.0 to 0.0
	float get_volume() const
	{
		float result;
		pump->execute_directly(msg_get_volume, &result);
		return result;
	}

	void set_volume(float vol)
	{
		std::auto_ptr<float> volume(new float(vol));
		pump->post_message_callback(msg_set_volume, volume.release(), this, &player_direct_show::async_callback, NULL);
	}

	// linearized such that 0 dB = 1, -6 dB ~= 0.5, -inf dB = 0.0 (except directshow is annoying, and doesn't go to -inf, only -100)
	float get_linear_volume() const
	{
		float raw_volume(get_volume());
		if(raw_volume <= -100.0)
		{
			return 0;
		}
		return std::pow(10.0f, raw_volume / 20.0f);
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

	boost::shared_ptr<utility::critical_section::attempt_lock> attempt_lock()
	{
		return boost::shared_ptr<utility::critical_section::attempt_lock>(new utility::critical_section::attempt_lock(cs));
	}

	// TODO make allocator non-public
	allocator_presenter* allocator;

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
	void async_callback(ULONG_PTR msg, void* arg, void* context, void* result)
	{
		switch(msg)
		{
		case msg_set_playback_position:
			{
				std::auto_ptr<float> pos(static_cast<float*>(arg));
			}
			break;
		case msg_set_volume:
			{
				std::auto_ptr<float> vol(static_cast<float*>(arg));
			}
			break;
		default:
			break;
		}
	}

	void set_allocator_presenter(IBaseFilterPtr filter, HWND window);
	void create_graph()
	{
		pump->send_message(msg_create_graph, NULL);
	}

	void destroy_graph()
	{
		pump->send_message(msg_destroy_graph, NULL);
	}

	void register_graph(IUnknownPtr unknown);
	void unregister_graph();

	std::auto_ptr<message_pump_type> pump;
	std::auto_ptr<event_handler_type> handler;

	void* do_msg_initialize_filter_graph(void*);
	void* do_msg_create_graph(void*);
	void* do_msg_destroy_graph(void*);
	void* do_msg_destroy_filter_graph(void*);
	void* do_msg_load(void*);
	void* do_msg_unload(void*);
	void* do_msg_ffwd(void*);
	void* do_msg_rwnd(void*);
	void* do_msg_pause(void*);
	void* do_msg_play(void*);
	void* do_msg_resume(void*);
	void* do_msg_stop(void*);
	void* do_msg_get_filters(void*);
	void* do_msg_get_linear_volume(void*);
	void* do_msg_set_linear_volume(void*);
	void* do_msg_get_volume(void*);
	void* do_msg_set_volume(void*);
	void* do_msg_get_movie_state(void*);
	void* do_msg_get_play_time(void*);
	void* do_msg_get_playback_position(void*);
	void* do_msg_set_playback_position(void*);
	void* do_msg_get_total_time(void*);

	static const transition_type transitions[max_graph_states][max_graph_events];

	size_t do_load()
	{
		size_t next_state;
		pump->send_message(msg_load, &next_state);
		return next_state;
	}
	size_t do_stop()
	{
		size_t next_state;
		pump->send_message(msg_stop, &next_state);
		return next_state;
	}
	size_t do_pause()
	{
		size_t next_state;
		pump->send_message(msg_pause, &next_state);
		return next_state;
	}
	size_t do_resume()
	{
		size_t next_state;
		pump->send_message(msg_resume, &next_state);
		return next_state;
	}
	size_t do_play()
	{
		size_t next_state;
		pump->send_message(msg_play, &next_state);
		return next_state;
	}
	size_t do_unload()
	{
		size_t next_state;
		pump->send_message(msg_unload, &next_state);
		return next_state;
	}
	size_t do_rwnd()
	{
		size_t next_state;
		pump->send_message(msg_rwnd, &next_state);
		return next_state;
	}
	size_t do_ffwd()
	{
		size_t next_state;
		pump->send_message(msg_ffwd, &next_state);
		return next_state;
	}

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

	utility::critical_section cs;

	IVMRSurfaceAllocator9Ptr vmr_surface_allocator;
};

#endif
