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
#include "util.h"

#include "direct3d.h"
#include "state_machine.h"
#include "player_playlist.h"
#include "player_window.h"
#include "player_scene.h"
#include "player_overlay.h"

struct player_direct_show;

_COM_SMARTPTR_TYPEDEF(IBaseFilter, __uuidof(IBaseFilter));

struct awkawk : boost::noncopyable
{
	awkawk();
	~awkawk();

	enum awkawk_state
	{
		unloaded,
		stopped,
		paused,
		playing,
		max_awkawk_states
	};

	std::string state_name(awkawk_state st) const
	{
		switch(st)
		{
		case unloaded:
			return "awkawk::unloaded";
		case stopped:
			return "awkawk::stopped";
		case paused:
			return "awkawk::paused";
		case playing:
			return "awkawk::playing";
		}
		::DebugBreak();
		return "awkawk::error";
	}

	enum awkawk_event
	{
		load,
		stop,
		pause,
		play,
		unload,
		transitioning,
		previous,
		next,
		rwnd,
		ffwd,
		max_awkawk_events
	};

	std::string event_name(awkawk_event evt) const
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
		case transitioning:
			return"transitioning";
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

	typedef transition<awkawk, awkawk_state> transition_type;
	typedef event_handler<awkawk, awkawk_state, awkawk_event> event_handler_type;

	const transition_type* get_transition(awkawk_event evt)
	{
		return &(transitions[get_current_state()][evt]);
	}

	awkawk_state send_event(awkawk_event evt)
	{
		return handler->send_event(evt);
	}

	void post_event(awkawk_event evt)
	{
		return handler->post_event(evt);
	}

	bool permitted(awkawk_event evt) const
	{
		return handler->permitted(evt);
	}

	void create_ui(int cmdShow);
	int run_ui();
	void render();
	void reset();

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


	void set_render_fps(unsigned int fps_)
	{
		LOCK(player_cs);
		fps = clamp(fps_, 25u, 60u);
	}

	void schedule_render() const
	{
		schedule_render(1.0f);
	}

	void schedule_render(float frames_ahead) const
	{
		LARGE_INTEGER dueTime = { 0 };
		dueTime.QuadPart = static_cast<LONGLONG>(frames_ahead * (-10000000.0f / static_cast<float>(get_render_fps())));
		::SetWaitableTimer(render_timer, &dueTime, 0, NULL, NULL, FALSE);
	}

	unsigned int get_render_fps() const
	{
		return fps;
	}

	const window* get_ui() const
	{
		return ui.get();
	}

	window* get_ui()
	{
		return ui.get();
	}

	void signal_new_frame() const
	{
		::SetEvent(render_event);
	}

	void signal_new_frame_and_wait() const
	{
		//::SetEvent(render_and_wait_event);
		//::WaitForSingleObject(render_and_wait_event, INFINITE);
		::SignalObjectAndWait(render_and_wait_event, render_finished_event, INFINITE, FALSE);
	}

	void stop_rendering();

	IDirect3DDevice9Ptr get_scene_device()
	{
		return scene_device;
	}

	awkawk_state get_awkawk_state() const { return current_state; }
	awkawk_state get_current_state() const { return current_state; }
	void set_current_state(awkawk_state st) { current_state = st; }

	void add_file(const std::wstring& path)
	{
		plist->add_file(path);
	}

	void clear_files()
	{
		plist->clear_files();
	}

	player_playlist::play_mode get_playmode() const
	{
		return plist->get_playmode();
	}

	void set_playmode(player_playlist::play_mode pm)
	{
		plist->set_playmode(pm);
	}

	std::wstring get_file_name() const
	{
		return plist->get_file_name();
	}

	void open_single_file(const std::wstring& path)
	{
		if(get_current_state() != unloaded)
		{
			if(get_current_state() != stopped)
			{
				send_event(stop);
			}
			send_event(unload);
			plist->clear_files();
		}
		plist->add_file(path);
		plist->do_next();
		post_event(load);
		post_event(play);
	}

	void set_linear_volume(float vol);
	void set_playback_position(float pos);
	std::vector<ATL::CAdapt<IBaseFilterPtr> > get_filters() const;

private:
	// D3D methods
	void create_d3d();
	void destroy_d3d();
	void create_device();
	void destroy_device();
	void reset_device();
	bool needs_display_change() const;

	static const transition_type transitions[max_awkawk_states][max_awkawk_events];

	size_t do_load();
	size_t do_stop();
	size_t do_pause();
	size_t do_resume();
	size_t do_play();
	size_t do_unload();
	size_t do_transition();
	size_t do_previous();
	size_t do_next();
	size_t do_rwnd();
	size_t do_ffwd();

	awkawk_state current_state;
	std::auto_ptr<event_handler_type> handler;

	std::auto_ptr<player_playlist> plist;
	std::auto_ptr<player_direct_show> dshow;

	// rendering
	HANDLE render_timer;
	HANDLE render_event;
	HANDLE render_and_wait_event;
	HANDLE render_finished_event;
	HANDLE cancel_render;
	HANDLE render_thread;
	DWORD render_thread_proc(void*);
	unsigned int fps;

	std::auto_ptr<player_window> ui;

	// direct3d gubbins
	std::auto_ptr<direct3d9> d3d9;

	IDirect3DDevice9Ptr scene_device;
	IDirect3DDevice9ExPtr scene_device_ex;

	D3DPRESENT_PARAMETERS presentation_parameters;
	void set_device_state();

	std::auto_ptr<player_scene> scene;
	std::auto_ptr<player_overlay> overlay;

	// video stats needed for controlling the window
	mutable utility::critical_section player_cs;

	SIZE video_size; // true size of the video
	SIZE scene_size; // size of the actual scene; in windowed mode, the same as the window size, in full-screen potentially smaller
	SIZE window_size; // true size of the window

	window_size_mode wnd_size_mode;
	size_t chosen_ar;
	size_t chosen_lb;

	bool fullscreen;
};

#endif
