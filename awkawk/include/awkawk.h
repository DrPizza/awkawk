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
#include "aspect_ratio.h"

#include "utility/interlocked_containers.hpp"

struct player_direct_show;
struct d3d_renderer;
struct shared_texture_queue;

_COM_SMARTPTR_TYPEDEF(IBaseFilter, __uuidof(IBaseFilter));
_COM_SMARTPTR_TYPEDEF(IDirect3DDeviceManager9, __uuidof(IDirect3DDeviceManager9));

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

	void send_event(awkawk_event evt)
	{
		return handler->send_event(evt);
	}

	void post_event(awkawk_event evt)
	{
		return handler->post_event(evt);
	}

	void post_callback(std::function<void()> f)
	{
		return handler->post_callback(f);
	}

	bool permitted(awkawk_event evt) const
	{
		return handler->permitted(evt);
	}

	void create_ui(int cmdShow);
	int run_ui();

	void set_cursor_position(const POINT& pos)
	{
		utility::critical_section::attempt_lock l(player_cs);
		if(l.succeeded && scene.get() != nullptr)
		{
			scene->set_cursor_position(pos);
		}
	}

	std::vector<std::shared_ptr<aspect_ratio> > available_ratios;

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

	std::vector<std::shared_ptr<letterbox> > available_letterboxes;

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

	rational_type get_size_multiplier() const
	{
		switch(wnd_size_mode)
		{
		case free:
			return rational_type(window_size.cx, video_size.cx);
		case fifty_percent:
			return rational_type(1, 2);
		case one_hundred_percent:
			return rational_type(1, 1);
		case two_hundred_percent:
			return rational_type(2, 1);
		default:
			__assume(0);
		}
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

	static SIZE fit_to_constraint(SIZE current_size, SIZE constraint, bool constrain_maximum_lengths)
	{
		rational_type current_ar(current_size.cx, current_size.cy);
		rational_type constraint_ar(constraint.cx, constraint.cy);
		
		return fix_ar(constraint, constraint_ar, current_ar, constrain_maximum_lengths);
	}

	void apply_sizing_policy()
	{
		LOCK(player_cs);

		// we have several sizes:
		// the video's natural size
		// the window's size
		// the AR-fixed video's size (video size * available_ratios[chosen_ar])
		// the AR-fixed, cropped video's size (video size * available_ratios[chosen_ar] * available_letterboxes[chosen_lb]
		// the screen's size
		// the D3D scene's size

		// the video is scaled as big as it needs to be to fit the size multiplier and AR fix
		// the scene is oversized, with letterbox black bars overflowing.
		// in windowed mode, we do this by leaving the scene size as-is, and then making the window undersized
		// in fullscreen mode, we do this by making the scene even bigger

		// windowed modes have a final scaling applied

		SIZE vid(get_video_dimensions());
		rational_type natural_video_ar(vid.cx, vid.cy);
		SIZE ar_fixed_vid(fix_ar(vid, natural_video_ar, available_ratios[chosen_ar]->get_multiplier(), false));
		SIZE protected_area(fix_ar(ar_fixed_vid, available_ratios[chosen_ar]->get_multiplier(), available_letterboxes[chosen_lb]->get_multiplier(), true));

		get_ui()->set_aspect_ratio(available_letterboxes[chosen_lb]->get_multiplier());

		if(is_fullscreen())
		{
			SIZE wnd_size(get_ui()->get_window_size());
			SIZE scaled_protected_area(fit_to_constraint(protected_area, wnd_size, true ));
			rational_type scaling_factor(scaled_protected_area.cx, protected_area.cx);
			SIZE scaled_video(scale_size(ar_fixed_vid, scaling_factor));

#if 0
			dout << "The video has a natural size of " << vid
			     << ", a corrected size of " << ar_fixed_vid
			     << ", with protected area of " << protected_area
			     << ", in a fullscreen window of of " << wnd_size << std::endl;
			dout << "The protected area needs to grow to " << scaled_protected_area
			     << ", requiring a total movie area of " << scaled_video << std::endl;
#endif

			set_scene_dimensions(scaled_video);
		}
		else
		{
			SIZE scaled_ar_fixed(scale_size(ar_fixed_vid, get_size_multiplier()));
			SIZE scaled_protected_area(scale_size(protected_area, get_size_multiplier()));

#if 0
			dout << "The video has a natural size of " << vid
			     << ", a rendered size of " << scaled_ar_fixed
			     << ", in a windowed window of of " << scaled_protected_area << std::endl;
#endif

			set_scene_dimensions(scaled_ar_fixed);
			get_ui()->resize_window(scaled_protected_area.cx, scaled_protected_area.cy);
		}

		if(scene.get() != nullptr)
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

	const player_window* get_ui() const
	{
		return ui.get();
	}

	player_window* get_ui()
	{
		return ui.get();
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
	static const transition_type transitions[max_awkawk_states][max_awkawk_events];

	awkawk_state do_load();
	awkawk_state do_stop();
	awkawk_state do_pause();
	awkawk_state do_resume();
	awkawk_state do_play();
	awkawk_state do_unload();
	awkawk_state do_transition();
	awkawk_state do_previous();
	awkawk_state do_next();
	awkawk_state do_track_change(void (player_playlist::*change_fn)(void));
	awkawk_state do_rwnd();
	awkawk_state do_ffwd();

	awkawk_state current_state;
	std::unique_ptr<event_handler_type> handler;

	std::unique_ptr<d3d_renderer> renderer;

	std::unique_ptr<player_playlist> plist;
	std::unique_ptr<player_direct_show> dshow;

	std::unique_ptr<shared_texture_queue> texture_queue;

	std::unique_ptr<player_window> ui;

	std::unique_ptr<player_scene> scene;
	std::unique_ptr<player_overlay> overlay;

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
