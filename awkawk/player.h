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

#ifndef PLAYER__H
#define PLAYER__H

#include "stdafx.h"
#include "resource.h"
#include "util.h"

#include "player_window.h"
#include "player_scene.h"
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

struct Player
{
	Player();
	~Player();

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

	void load();
	void close();
	void play();
	void pause();
	void stop();
	void ffwd();
	void next();
	void rwnd();
	void prev();

	enum status
	{
		unloaded,
		loading,
		stopped,
		playing,
		paused
	};

	status get_state() const
	{
		return state;
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
	void create_device();
	void destroy_device();
	void reset_device();
	bool needs_display_change() const;

	std::vector<CAdapt<IBaseFilterPtr> > get_filters();
	std::vector<std::wstring> get_filter_names()
	{
		std::vector<CAdapt<IBaseFilterPtr> > filters(get_filters());
		std::vector<std::wstring> rv;
		for(std::vector<CAdapt<IBaseFilterPtr> >::iterator it(filters.begin()), end(filters.end()); it != end; ++it)
		{
			FILTER_INFO fi = {0};
			static_cast<IBaseFilterPtr&>(*it)->QueryFilterInfo(&fi);
			IFilterGraphPtr ptr(fi.pGraph, false);
			rv.push_back(fi.achName);
		}
		return rv;
	}

	std::vector<std::wstring> get_filter_vendor_info()
	{
		std::vector<CAdapt<IBaseFilterPtr> > filters(get_filters());
		std::vector<std::wstring> rv;
		for(std::vector<CAdapt<IBaseFilterPtr> >::iterator it(filters.begin()), end(filters.end()); it != end; ++it)
		{
			wchar_t* info(NULL);
			static_cast<IBaseFilterPtr&>(*it)->QueryVendorInfo(&info);
			ON_BLOCK_EXIT(::CoTaskMemFree, info);
			if(info != NULL)
			{
				rv.push_back(info);
			}
		}
		return rv;
	}

	void set_cursor_position(const POINT& pos)
	{
		critical_section::attempt_lock l(player_cs);
		if(l.succeeded && scene.get() != NULL)
		{
			scene->set_cursor_position(pos);
		}
	}

	enum aspect_ratio_mode
	{
		original = IDM_AR_ORIGINAL,
		onethreethree_to_one = IDM_AR_133TO1,
		onefivefive_to_one = IDM_AR_155TO1,
		onesevenseven_to_one = IDM_AR_177TO1,
		oneeightfive_to_one = IDM_AR_185TO1,
		twofourzero_to_one = IDM_AR_240TO1
	};

	aspect_ratio_mode get_aspect_ratio_mode() const
	{
		return ar_mode;
	}

	void set_aspect_ratio_mode(aspect_ratio_mode mode)
	{
		critical_section::lock l(player_cs);
		ar_mode = mode;
		apply_sizing_policy();
	}

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
		critical_section::lock l(player_cs);
		wnd_size_mode = mode;
		apply_sizing_policy();
	}

	enum letterbox_mode
	{
		no_letterboxing = IDM_NOLETTERBOXING,
		four_to_three_original = IDM_4_TO_3_ORIGINAL,
		fourteen_to_nine_original = IDM_14_TO_9_ORIGINAL,
		sixteen_to_nine_original = IDM_16_TO_9_ORIGINAL,
		oneeightfive_to_one_original = IDM_185_TO_1_ORIGINAL,
		twofourzero_to_one_original = IDM_240_TO_1_ORIGINAL
	};

	letterbox_mode get_letterbox_mode() const
	{
		return ltrbx_mode;
	}

	void set_letterbox_mode(letterbox_mode mode)
	{
		critical_section::lock l(player_cs);
		ltrbx_mode = mode;
		apply_sizing_policy();
	}

	double get_size_multiplier() const
	{
		switch(wnd_size_mode)
		{
		case free:
			return static_cast<double>(window_size.cx) / static_cast<double>(video_size.cx);
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

	double get_aspect_ratio() const
	{
		switch(ar_mode)
		{
		case original:
			return static_cast<double>(video_size.cx) / static_cast<double>(video_size.cy);
		case onethreethree_to_one:
			return 4.0 / 3.0;
		case onefivefive_to_one:
			return 14.0 / 9.0;
		case onesevenseven_to_one:
			return 16.0 / 9.0;
		case oneeightfive_to_one:
			return 1.85;
		case twofourzero_to_one:
			return 2.40;
		default:
			__assume(0);
		}
	}

	void set_window_dimensions(SIZE sz)
	{
		critical_section::lock l(player_cs);
		if(window_size.cx != sz.cx || window_size.cy != sz.cy)
		{
			window_size = sz;
			apply_sizing_policy();
		}
	}

	void size_window_from_video()
	{
		critical_section::lock l(player_cs);
		SIZE new_size = { static_cast<LONG>(static_cast<double>(video_size.cx) * get_size_multiplier()),
		                  static_cast<LONG>(static_cast<double>(video_size.cy) * get_size_multiplier()) };

		if(static_cast<double>(new_size.cx) / static_cast<double>(new_size.cx) > get_aspect_ratio())
		{
			new_size.cy = static_cast<LONG>(static_cast<float>(new_size.cx) / get_aspect_ratio());
		}
		else if(static_cast<double>(new_size.cx) / static_cast<double>(new_size.cx) < get_aspect_ratio())
		{
			new_size.cx = static_cast<LONG>(static_cast<float>(new_size.cy) * get_aspect_ratio());
		}
		switch(get_letterbox_mode())
		{
		case no_letterboxing:
			break;
		case four_to_three_original:
			if(get_aspect_ratio() > 4.0 / 3.0)
			{
				new_size.cx = static_cast<LONG>(static_cast<float>(new_size.cy) * (4.0f / 3.0f));
			}
			else
			{
				new_size.cy = static_cast<LONG>(static_cast<float>(new_size.cx) / (4.0f / 3.0f));
			}
			break;
		case fourteen_to_nine_original:
			if(get_aspect_ratio() > 14.0 / 9.0)
			{
				new_size.cx = static_cast<LONG>(static_cast<float>(new_size.cy) * (14.0f / 9.0f));
			}
			else
			{
				new_size.cy = static_cast<LONG>(static_cast<float>(new_size.cx) / (14.0f / 9.0f));
			}
			break;
		case sixteen_to_nine_original:
			if(get_aspect_ratio() > 16.0 / 9.0)
			{
				new_size.cx = static_cast<LONG>(static_cast<float>(new_size.cy) * (16.0f / 9.0f));
			}
			else
			{
				new_size.cy = static_cast<LONG>(static_cast<float>(new_size.cx) / (16.0f / 9.0f));
			}
			break;
		case oneeightfive_to_one_original:
			if(get_aspect_ratio() > 1.85)
			{
				new_size.cx = static_cast<LONG>(static_cast<float>(new_size.cy) * (1.85f));
			}
			else
			{
				new_size.cy = static_cast<LONG>(static_cast<float>(new_size.cx) / (1.85f));
			}
			break;
		case twofourzero_to_one_original:
			if(get_aspect_ratio() > 2.40)
			{
				new_size.cx = static_cast<LONG>(static_cast<float>(new_size.cy) * (2.40f));
			}
			else
			{
				new_size.cy = static_cast<LONG>(static_cast<float>(new_size.cx) / (2.40f));
			}
			break;

		}
		window_size = new_size;
		ui.resize_window(window_size.cx, window_size.cy);
	}

	void size_window_from_screen()
	{
		critical_section::lock l(player_cs);
		WINDOWPLACEMENT placement(ui.get_placement());
		placement.rcNormalPosition.right = placement.rcNormalPosition.left + video_size.cx;
		placement.rcNormalPosition.bottom = placement.rcNormalPosition.top + video_size.cy;
		ui.set_placement(&placement);
	}

	void size_scene_from_window()
	{
		critical_section::lock l(player_cs);
		scene_size = window_size;
	}

	void size_scene_from_screen()
	{
		critical_section::lock l(player_cs);
		float window_ar(static_cast<float>(window_size.cx) / static_cast<float>(window_size.cy));
		float video_ar(static_cast<float>(get_aspect_ratio()));
		switch(get_letterbox_mode())
		{
		case no_letterboxing:
			break;
		case four_to_three_original:
			video_ar = 4.0f / 3.0f;
			break;
		case fourteen_to_nine_original:
			video_ar = 14.0f / 9.0f;
			break;
		case sixteen_to_nine_original:
			video_ar = 16.0f / 9.0f;
			break;
		case oneeightfive_to_one_original:
			video_ar = 1.85f;
			break;
		case twofourzero_to_one_original:
			video_ar = 2.40f;
			break;
		}
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
		switch(get_letterbox_mode())
		{
		case no_letterboxing:
			break;
		case four_to_three_original:
			if(get_aspect_ratio() > 4.0 / 3.0)
			{
				scene_size.cx = static_cast<LONG>(static_cast<float>(scene_size.cy) * get_aspect_ratio());
			}
			else
			{
				scene_size.cy = static_cast<LONG>(static_cast<float>(scene_size.cx) / get_aspect_ratio());
			}
			break;
		case fourteen_to_nine_original:
			if(get_aspect_ratio() > 14.0 / 9.0)
			{
				scene_size.cx = static_cast<LONG>(static_cast<float>(scene_size.cy) * get_aspect_ratio());
			}
			else
			{
				scene_size.cy = static_cast<LONG>(static_cast<float>(scene_size.cx) / get_aspect_ratio());
			}
			break;
		case sixteen_to_nine_original:
			if(get_aspect_ratio() > 16.0 / 9.0)
			{
				scene_size.cx = static_cast<LONG>(static_cast<float>(scene_size.cy) * get_aspect_ratio());
			}
			else
			{
				scene_size.cy = static_cast<LONG>(static_cast<float>(scene_size.cx) / get_aspect_ratio());
			}
			break;
		case oneeightfive_to_one_original:
			if(get_aspect_ratio() > 1.85)
			{
				scene_size.cx = static_cast<LONG>(static_cast<float>(scene_size.cy) * get_aspect_ratio());
			}
			else
			{
				scene_size.cy = static_cast<LONG>(static_cast<float>(scene_size.cx) / get_aspect_ratio());
			}
			break;
		case twofourzero_to_one_original:
			if(get_aspect_ratio() > 2.40)
			{
				scene_size.cx = static_cast<LONG>(static_cast<float>(scene_size.cy) * get_aspect_ratio());
			}
			else
			{
				scene_size.cy = static_cast<LONG>(static_cast<float>(scene_size.cx) / get_aspect_ratio());
			}
			break;
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
		critical_section::lock l(player_cs);
		scene_size = sz;
	}

	SIZE get_scene_dimensions() const
	{
		return scene_size;
	}

	void set_video_dimensions(SIZE sz)
	{
		critical_section::lock l(player_cs);
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
			critical_section::lock l(graph_cs);
			LONGLONG end(0);
			seeking->GetStopPosition(&end);
			LONGLONG position(static_cast<LONGLONG>(percentage * static_cast<float>(end) / 100.0f));
			seeking->SetPositions(&position, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
		}
	}

	float get_playback_position() const
	{
		try
		{
			critical_section::lock l(graph_cs);
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
			critical_section::lock l(graph_cs);
			LONG volume(0);
			audio->get_Volume(&volume);
			return (static_cast<float>(volume) / static_cast<float>(100));
		}
		return -std::numeric_limits<float>::infinity();
	}

	void set_volume(float vol)
	{
		if(get_state() != unloaded)
		{
			critical_section::lock l(graph_cs);
			LONG volume(static_cast<LONG>(vol * static_cast<float>(100)));
			audio->put_Volume(volume);
		}
	}

	// linearized such that 0 dB = 1, -6 dB ~= 0.5, -inf dB = 0.0 (except directshow is annoying, and doesn't go to -inf, only -100)
	float get_linear_volume() const
	{
		float raw_volume(get_volume());
		if(raw_volume <= -100.0f)
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

	void toggle_fullscreen()
	{
		critical_section::lock l(player_cs);
		fullscreen = !fullscreen;
	}

	std::wstring get_file_name() const
	{
		return *playlist_position;
	}

	void set_render_fps(unsigned int fps_)
	{
		critical_section::lock l(player_cs);
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

	HWND get_window() const
	{
		return ui.get_window();
	}

	void signal_new_frame() const
	{
		::SetEvent(render_event);
	}

	typedef std::list<std::wstring> playlist_type;

private:
	void set_allocator_presenter(IBaseFilterPtr filter, HWND window);
	void create_graph();
	void destroy_graph();

	REFERENCE_TIME get_average_frame_time(IFilterGraphPtr grph) const;
	SIZE get_video_size(IFilterGraphPtr grph) const;

	void register_graph(IUnknownPtr unknown);
	void unregister_graph();

	HANDLE event;
	HANDLE cancel_event;
	HANDLE event_thread;
	DWORD event_thread_proc(void*);

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
	mutable critical_section graph_cs;

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
	surface_allocator* allocator;
	player_window ui;

	// direct3d gubbins
	IDirect3D9Ptr d3d;
	IDirect3DDevice9Ptr device;
	D3DPRESENT_PARAMETERS presentation_parameters;
	IDirect3DSurface9Ptr render_target;

	std::auto_ptr<player_scene> scene;

	// video stats needed for controlling the window
	mutable critical_section player_cs;

	SIZE video_size; // true size of the video
	SIZE scene_size; // size of the actual scene; in windowed mode, the same as the window size, in full-screen potentially smaller
	SIZE window_size; // true size of the window

	window_size_mode wnd_size_mode;
	aspect_ratio_mode ar_mode;
	letterbox_mode ltrbx_mode;

	bool fullscreen;

	status state;

	playlist_type playlist;
	playlist_type::iterator playlist_position;
	play_mode playlist_mode;

//	std::wstring file;
};

#endif
