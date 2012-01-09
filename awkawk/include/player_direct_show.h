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
#include "vmr9_allocator.h"

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

struct shared_texture_queue;

struct player_direct_show : boost::noncopyable
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

	const transition_type* get_transition(graph_event evt) const
	{
		return &(transitions[get_current_state()][evt]);
	}

	player_direct_show(awkawk* player_,
	                   shared_texture_queue* texture_queue_,
	                   d3d_renderer* renderer_,
	                   HWND window_);

	~player_direct_show();

	void send_event(graph_event evt)
	{
		handler->send_event(evt);
	}

	void post_event(graph_event evt)
	{
		handler->post_event(evt);
	}

	void post_event_with_callback(graph_event evt, std::function<void()> f) {
		handler->post_event_with_callback(evt, f);
	}

	void post_callback(std::function<void()> f)
	{
		handler->post_callback(f);
	}

	graph_state get_graph_state() const { return current_state; }
	graph_state get_current_state() const { return current_state; }
	void set_current_state(graph_state st) { current_state = st; }

	float get_volume_unsync() const;
	float get_play_time_unsync() const;
	float get_playback_position_unsync() const;
	float get_total_time_unsync() const;

	// cross-thread !!synchronous!! getters
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

	std::vector<ATL::CAdapt<IBaseFilterPtr> > get_filters() const
	{
		std::vector<ATL::CAdapt<IBaseFilterPtr> > result;
		handler->send_callback([&] {
			result = do_get_filters();
		});
		return result;
	}

	float get_play_time() const
	{
		float result;
		handler->send_callback([&] {
			result = get_play_time_unsync();
		});
		return result;
	}

	float get_total_time() const
	{
		float result;
		handler->send_callback([&] {
			result = get_total_time_unsync();
		});
		return result;
	}

	float get_playback_position() const
	{
		float result;
		handler->send_callback([&] {
			result = get_playback_position_unsync();
		});
		return result;
	}

	// in dB, from -100.0 to 0.0
	float get_volume() const
	{
		float result;
		handler->send_callback([&] {
			result = get_volume_unsync();
		});
		return result;
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

	float get_linear_volume_unsync() const
	{
		float raw_volume(get_volume_unsync());
		if(raw_volume <= -100.0)
		{
			return 0;
		}
		return std::pow(10.0f, raw_volume / 20.0f);
	}

	// cross-thread setters
	void set_playback_position(float percentage)
	{
		handler->post_callback([=] {
			do_set_playback_position(percentage);
		});
	}

	void set_volume(float vol)
	{
		handler->post_callback([=] {
			do_set_volume(vol);
		});
	}

	void set_linear_volume(float vol)
	{
		vol = clamp(vol, 0.0f, 1.0f);
		vol = std::log10(vol) * 20.0f;
		vol = clamp(vol, -100.0f, 0.0f);
		set_volume(vol);
	}

	SIZE get_video_size() const;

	bool get_has_video() const
	{
		return has_video;
	}

	// TODO make allocator non-public
	vmr9_allocator_presenter* allocator;

private:
	void add_vmr9();
	void set_allocator_presenter(IBaseFilterPtr filter);

	void create_graph();

	void destroy_graph();

	void register_graph(IUnknownPtr unknown);
	void unregister_graph();

	void initialize();
	void uninitialize();

	std::vector<ATL::CAdapt<IBaseFilterPtr> > do_get_filters() const;
	void do_set_linear_volume(float);
	void do_set_volume(float);
	void do_set_playback_position(float);

	static const transition_type transitions[max_graph_states][max_graph_events];
	std::unique_ptr<event_handler_type> handler;

	graph_state do_load();
	graph_state do_stop();
	graph_state do_pause();
	graph_state do_resume();
	graph_state do_play();
	graph_state do_unload();
	graph_state do_rwnd();
	graph_state do_ffwd();

	awkawk* player;
	d3d_renderer* renderer;
	shared_texture_queue* texture_queue;
	HWND window;

	// graph state
	graph_state current_state;

	// ROT registration
	DWORD rot_key;

	HANDLE media_event;
	HANDLE cancel_media_event;
	HANDLE media_event_thread;
	DWORD media_event_thread_proc(void*);

	// DirectShow gubbins
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
