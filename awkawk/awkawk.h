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

#pragma once

#ifndef PLAYER__H
#define PLAYER__H

#include "stdafx.h"
#include "resource.h"
#include "util.h"

#include "player_window.h"
#include "player_scene.h"
#include "player_overlay.h"
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

struct awkawk : boost::noncopyable
{
	awkawk();
	~awkawk();

	void add_file(const std::wstring& path)
	{
		playlist.push_back(path);
		if(playlist.size() == 1)
		{
			playlist_position = playlist.begin();
		}
	}

	void clear_files()
	{
		playlist.clear();
		playlist_position = playlist.begin();
	}

	enum state
	{
		unloaded,
		stopped,
		paused,
		playing,
		max_states
	};

	state get_state() const { return current_state; }

	std::string state_name(state st) const
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

	enum event
	{
		load,
		stop,
		pause,
		play,
		unload,
		ending,
		previous,
		next,
		rwnd,
		ffwd,
		max_events
	};

	std::string event_name(event evt) const
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
		case ending:
			return"ending";
		case previous:
			return"previous";
		case next:
			return"next";
		case rwnd:
			return"rwnd";
		case ffwd:
			return"ffwd";
		}
		return "(not an event)";
	}

	void post_event(event evt)
	{
		dout << "posting " << event_name(evt) << " (" << std::hex << WM_USER + evt << ")" << std::endl;
		::PostQueuedCompletionStatus(message_port, static_cast<DWORD>(evt), 0, NULL);
		dout << "posted " << event_name(evt) << " (" << std::hex << WM_USER + evt << ")" << std::endl;
	}

	void send_event(event evt)
	{
		OVERLAPPED o = {0};
		o.hEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
		ON_BLOCK_EXIT(&::CloseHandle, o.hEvent);
		dout << "sending " << event_name(evt) << " (" << std::hex << WM_USER + evt << ")" << std::endl;
		::PostQueuedCompletionStatus(message_port, static_cast<DWORD>(evt), 0, &o);
		::WaitForSingleObject(o.hEvent, INFINITE);
		dout << "sent " << event_name(evt) << " (" << std::hex << WM_USER + evt << ")" << std::endl;
	}

	bool permitted(event evt) const
	{
		return transitions[current_state][evt].next_states.length != 0;
	}

	enum play_mode
	{
		normal = IDM_PLAYMODE_NORMAL, // advance through playlist, stop & rewind when the last file is played
		repeat_all = IDM_PLAYMODE_REPEATALL, // advance through the playlist, rewind & continue playing when the last file is played
		repeat_single = IDM_PLAYMODE_REPEATTRACK, // do not advance through playlist, rewind track & continue playing
		shuffle = IDM_PLAYMODE_SHUFFLE // advance through playlist randomly, no concept of a "last file"
	};

	play_mode get_playmode() const
	{
		return playlist_mode;
	}

	void set_playmode(play_mode pm)
	{
		playlist_mode = pm;
	}

	OAFilterState get_movie_state() const
	{
		OAFilterState state;
		FAIL_THROW(media_control->GetState(0, &state));
		return state;
	}

	void create_ui(int cmdShow);
	int run_ui();
	void render();
	void reset();

	// D3D methods
	void create_d3d();
	void destroy_d3d();
	void create_device();
	void destroy_device();
	void reset_device();
	bool needs_display_change() const;

	std::vector<CAdapt<IBaseFilterPtr> > get_filters()
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

	void set_cursor_position(const POINT& pos)
	{
		utility::critical_section::attempt_lock l(player_cs);
		if(l.succeeded && scene.get() != NULL)
		{
			scene->set_cursor_position(pos);
		}
	}

	struct aspect_ratio
	{
		virtual float get_muliplier() const = 0;

		virtual const std::wstring get_name() const = 0;

		virtual ~aspect_ratio()
		{
		}
	};

	struct fixed_aspect_ratio : aspect_ratio
	{
		fixed_aspect_ratio(unsigned int width_, unsigned int height_) : width(width_), height(height_), ratio(static_cast<float>(width) / static_cast<float>(height))
		{
		}

		fixed_aspect_ratio(float ratio_) : width(0), height(0), ratio(ratio_)
		{
		}

		virtual float get_muliplier() const
		{
			return ratio;
		}

		virtual const std::wstring get_name() const
		{
			std::wstringstream wss;
			if(width != 0 && height != 0)
			{
				wss << width << L":" << height;
			}
			else
			{
				wss << std::fixed << std::setprecision(2) << ratio << L":1";
			}
			return wss.str();
		}

	private:
		unsigned int width;
		unsigned int height;
		float ratio;
	};

	struct natural_aspect_ratio : aspect_ratio
	{
		natural_aspect_ratio(awkawk* player_) : player(player_)
		{
		}

		virtual float get_muliplier() const
		{
			SIZE sz(player->get_video_dimensions());
			return static_cast<float>(sz.cx) / static_cast<float>(sz.cy);
		}

		virtual const std::wstring get_name() const
		{
			std::wstringstream wss;
			wss << L"Original (" << std::fixed << std::setprecision(2) << get_muliplier() << L":1)";
			return wss.str();
		}

	private:
		awkawk* player;
	};

	std::vector<boost::shared_ptr<aspect_ratio> > available_ratios;

	size_t get_aspect_ratio_mode() const
	{
		return chosen_ar;
	}

	void set_aspect_ratio_mode(size_t new_ar)
	{
		LOCK(player_cs);
		chosen_ar = new_ar;
		apply_sizing_policy();
	}

	struct letterbox
	{
		virtual float get_muliplier() const = 0;

		virtual SIZE get_boxed_size(float actual_ar, SIZE current_size) const = 0;

		virtual const std::wstring get_name() const = 0;

		virtual ~letterbox()
		{
		}
	};

	struct fixed_letterbox : letterbox
	{
		fixed_letterbox(unsigned int width_, unsigned int height_) : width(width_), height(height_), ratio(static_cast<float>(width) / static_cast<float>(height))
		{
		}

		fixed_letterbox(float ratio_) : width(0), height(0), ratio(ratio_)
		{
		}

		virtual float get_muliplier() const
		{
			return ratio;
		}

		virtual SIZE get_boxed_size(float actual_ar, SIZE current_size) const
		{
			if(actual_ar > get_muliplier())
			{
				current_size.cx = static_cast<LONG>(static_cast<float>(current_size.cy) * get_muliplier());
			}
			else
			{
				current_size.cy = static_cast<LONG>(static_cast<float>(current_size.cx) / get_muliplier());
			}
			return current_size;
		}

		virtual const std::wstring get_name() const
		{
			std::wstringstream wss;
			if(width != 0 && height != 0)
			{
				wss << width << L":" << height;
			}
			else
			{
				wss << std::fixed << std::setprecision(2) << ratio << L":1";
			}
			wss << L" Original";
			return wss.str();
		}

	private:
		unsigned int width;
		unsigned int height;
		float ratio;
	};

	struct natural_letterbox : letterbox
	{
		natural_letterbox(awkawk* player_) : player(player_)
		{
		}

		virtual float get_muliplier() const
		{
			SIZE sz(player->get_video_dimensions());
			return static_cast<float>(sz.cx) / static_cast<float>(sz.cy);
		}

		virtual SIZE get_boxed_size(float actual_ar, SIZE current_size) const
		{
			return current_size;
		}

		virtual const std::wstring get_name() const
		{
			std::wstringstream wss;
			wss << L"No letterboxing (" << std::fixed << std::setprecision(2) << get_muliplier() << L":1)";
			return wss.str();
		}

	private:
		awkawk* player;
	};

	std::vector<boost::shared_ptr<letterbox> > available_letterboxes;

	enum window_size_mode
	{
		free = IDM_SIZE_FREE,
		fifty_percent = IDM_SIZE_50,
		one_hundred_percent = IDM_SIZE_100,
		two_hundred_percent = IDM_SIZE_200
	};

	window_size_mode get_window_size_mode() const
	{
		return wnd_size_mode;
	}

	void set_window_size_mode(window_size_mode mode)
	{
		LOCK(player_cs);
		wnd_size_mode = mode;
		apply_sizing_policy();
	}

	size_t get_letterbox_mode() const
	{
		return chosen_lb;
	}

	void set_letterbox_mode(size_t mode)
	{
		LOCK(player_cs);
		chosen_lb = mode;
		apply_sizing_policy();
	}

	float get_size_multiplier() const
	{
		switch(wnd_size_mode)
		{
		case free:
			return static_cast<float>(window_size.cx) / static_cast<float>(video_size.cx);
		case fifty_percent:
			return 0.5;
		case one_hundred_percent:
			return 1.0;
		case two_hundred_percent:
			return 2.0;
		default:
			__assume(0);
		}
	}

	float get_aspect_ratio() const
	{
		return available_ratios[chosen_ar]->get_muliplier();
	}

	void set_window_dimensions(SIZE sz)
	{
		LOCK(player_cs);
		if(window_size.cx != sz.cx || window_size.cy != sz.cy)
		{
			window_size = sz;
			apply_sizing_policy();
		}
	}

	void size_window_from_video()
	{
		LOCK(player_cs);
		SIZE new_size = { static_cast<LONG>(static_cast<float>(video_size.cx) * get_size_multiplier()),
		                  static_cast<LONG>(static_cast<float>(video_size.cy) * get_size_multiplier()) };

		if(static_cast<float>(new_size.cx) / static_cast<float>(new_size.cx) > get_aspect_ratio())
		{
			new_size.cy = static_cast<LONG>(static_cast<float>(new_size.cx) / get_aspect_ratio());
		}
		else if(static_cast<float>(new_size.cx) / static_cast<float>(new_size.cx) < get_aspect_ratio())
		{
			new_size.cx = static_cast<LONG>(static_cast<float>(new_size.cy) * get_aspect_ratio());
		}
		new_size = available_letterboxes[chosen_lb]->get_boxed_size(get_aspect_ratio(), new_size);
		window_size = new_size;
		get_ui()->resize_window(window_size.cx, window_size.cy);
	}

	void size_window_from_screen()
	{
		LOCK(player_cs);
		WINDOWPLACEMENT placement(get_ui()->get_placement());
		placement.rcNormalPosition.right = placement.rcNormalPosition.left + video_size.cx;
		placement.rcNormalPosition.bottom = placement.rcNormalPosition.top + video_size.cy;
		get_ui()->set_placement(&placement);
	}

	void size_scene_from_window()
	{
		LOCK(player_cs);
		scene_size = window_size;
	}

	void size_scene_from_screen()
	{
		LOCK(player_cs);
		float window_ar(static_cast<float>(window_size.cx) / static_cast<float>(window_size.cy));
		float video_ar(available_letterboxes[chosen_lb]->get_muliplier());
		if(window_ar > video_ar)
		{
			scene_size.cx = static_cast<LONG>(static_cast<float>(window_size.cy) * video_ar);
			scene_size.cy = static_cast<LONG>(window_size.cy);
		}
		else
		{
			scene_size.cx = static_cast<LONG>(window_size.cx);
			scene_size.cy = static_cast<LONG>(static_cast<float>(window_size.cx) / video_ar);
		}
	}

	void update_scene_dimensions()
	{
		if(is_fullscreen())
		{
			size_scene_from_screen();
		}
		else
		{
			size_scene_from_window();
		}
		if(get_aspect_ratio() > available_letterboxes[chosen_lb]->get_muliplier())
		{
			scene_size.cx = static_cast<LONG>(static_cast<float>(scene_size.cy) * get_aspect_ratio());
		}
		else
		{
			scene_size.cy = static_cast<LONG>(static_cast<float>(scene_size.cx) / get_aspect_ratio());
		}
	}

	void update_window_dimensions()
	{
		if(is_fullscreen())
		{
			size_window_from_screen();
		}
		else
		{
			size_window_from_video();
		}
	}

	void apply_sizing_policy()
	{
		update_window_dimensions();
		update_scene_dimensions();
		if(scene.get() != NULL)
		{
			scene->notify_window_size_change();
		}
	}

	SIZE get_window_dimensions() const
	{
		return window_size;
	}

	void set_scene_dimensions(SIZE sz)
	{
		LOCK(player_cs);
		scene_size = sz;
	}

	SIZE get_scene_dimensions() const
	{
		return scene_size;
	}

	void set_video_dimensions(SIZE sz)
	{
		LOCK(player_cs);
		if(video_size.cx != sz.cx || video_size.cy != sz.cy)
		{
			video_size = sz;
			apply_sizing_policy();
		}
	}

	SIZE get_video_dimensions() const
	{
		return video_size;
	}

	void set_playback_position(float percentage)
	{
		if(get_state() != unloaded)
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
			if(get_state() != unloaded)
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
			if(get_state() != unloaded)
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
			if(get_state() != unloaded)
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
		if(get_state() != unloaded)
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
		if(get_state() != unloaded)
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

	bool is_fullscreen() const
	{
		return fullscreen;
	}

	void set_fullscreen(bool fullscreen_)
	{
		LOCK(player_cs);
		fullscreen = fullscreen_;
	}

	void toggle_fullscreen()
	{
		set_fullscreen(!is_fullscreen());
	}

	std::wstring get_file_name() const
	{
		return *playlist_position;
	}

	void set_render_fps(unsigned int fps_)
	{
		LOCK(player_cs);
		fps = clamp(fps_, 25u, 60u);
	}

	void schedule_render() const
	{
		LARGE_INTEGER dueTime = { 0 };
		dueTime.QuadPart = -10000000 / static_cast<LONGLONG>(get_render_fps());
		::SetWaitableTimer(render_timer, &dueTime, 0, NULL, NULL, FALSE);
	}

	unsigned int get_render_fps() const
	{
		return fps;
	}

	const window* get_ui() const
	{
		return &ui;
	}

	window* get_ui()
	{
		return &ui;
	}

	void signal_new_frame() const
	{
		::SetEvent(render_event);
	}

	void stop_rendering();

	typedef std::list<std::wstring> playlist_type;

private:
	state current_state;

	typedef size_t (awkawk::*state_fun)(void);
	typedef array<state> state_array;

	friend struct transition;

	struct transition
	{
		state_fun fun;
		array<state> next_states;

		state execute(awkawk* awk) const
		{
			return next_states.length > 0 ? next_states[(awk->*fun)()]
			                              : awk->current_state;
		}
	};

	static const transition transitions[max_states][max_events];

	size_t do_load();
	size_t do_stop();
	size_t do_pause();
	size_t do_resume();
	size_t do_play();
	size_t do_unload();
	size_t do_ending();
	size_t do_previous();
	size_t do_next();
	size_t do_rwnd();
	size_t do_ffwd();

	void set_allocator_presenter(IBaseFilterPtr filter, HWND window);
	void create_graph();
	void destroy_graph();

	REFERENCE_TIME get_average_frame_time(IFilterGraphPtr grph) const;
	SIZE get_video_size() const;

	void register_graph(IUnknownPtr unknown);
	void unregister_graph();

	HANDLE media_event;
	HANDLE cancel_media_event;
	HANDLE media_event_thread;
	DWORD media_event_thread_proc(void*);

	HANDLE message_port;
	HANDLE message_thread;
	DWORD message_thread_proc(void*);

	::tm convert_win32_time(LONGLONG w32Time);

	// rendering
	HANDLE render_timer;
	HANDLE render_event;
	HANDLE cancel_render;
	HANDLE render_thread;
	DWORD render_thread_proc(void*);
	unsigned int fps;

	// ROT registration
	DWORD rot_key;

	// DirectShow gubbins
	mutable utility::critical_section graph_cs;

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
	allocator_presenter* allocator;
	player_window ui;

	// direct3d gubbins
	IDirect3D9Ptr d3d;
	IDirect3DDevice9Ptr scene_device;
	D3DPRESENT_PARAMETERS presentation_parameters;
	IDirect3DSurface9Ptr render_target;

	std::auto_ptr<player_scene> scene;
	std::auto_ptr<player_overlay> overlay;

	// video stats needed for controlling the window
	mutable utility::critical_section player_cs;

	SIZE video_size; // true size of the video
	SIZE scene_size; // size of the actual scene; in windowed mode, the same as the window size, in full-screen potentially smaller
	SIZE window_size; // true size of the window

	window_size_mode wnd_size_mode;
	size_t chosen_ar;
	//letterbox_mode ltrbx_mode;
	size_t chosen_lb;

	bool fullscreen;

	playlist_type playlist;
	playlist_type::iterator playlist_position;
	play_mode playlist_mode;

//	std::wstring file;
};

#endif
