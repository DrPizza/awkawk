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
#include "shared_texture_queue.h"
#include "player_direct_show.h"
#include "d3d_renderer.h"

const awkawk::transition_type awkawk::transitions[awkawk::max_awkawk_states][awkawk::max_awkawk_events] =
{
/* state    | event              | handler                | exit states                     */
/* ------------------------ */ {
/* unloaded | load          */   { &awkawk::do_load       /* stopped                        */},
/*          | stop          */   { nullptr                /*                                */},
/*          | pause         */   { nullptr                /*                                */},
/*          | play          */   { nullptr                /*                                */},
/*          | unload        */   { nullptr                /*                                */},
/*          | transitioning */   { nullptr                /*                                */},
/*          | previous      */   { &awkawk::do_previous   /* unloaded << stopped << playing */},
/*          | next          */   { nullptr                /*                                */},
/*          | rwnd          */   { nullptr                /*                                */},
/*          | ffwd          */   { nullptr                /*                                */}
                          },
                          {
/* stopped  | load          */   { nullptr                /*                                */},
/*          | stop          */   { nullptr                /*                                */},
/*          | pause         */   { &awkawk::do_play       /* playing                        */},
/*          | play          */   { &awkawk::do_play       /* playing                        */},
/*          | unload        */   { &awkawk::do_unload     /* unloaded                       */},
/*          | transitioning */   { nullptr                /*                                */},
/*          | previous      */   { &awkawk::do_previous   /* unloaded << stopped << playing */},
/*          | next          */   { &awkawk::do_next       /* unloaded << stopped << playing */},
/*          | rwnd          */   { nullptr                /*                                */},
/*          | ffwd          */   { nullptr                /*                                */}
                          },
                          {
/* paused   | load          */   { nullptr                /*                                */},
/*          | stop          */   { &awkawk::do_stop       /* stopped                        */},
/*          | pause         */   { &awkawk::do_resume     /* playing                        */},
/*          | play          */   { &awkawk::do_resume     /* playing                        */},
/*          | unload        */   { nullptr                /*                                */},
/*          | transitioning */   { nullptr                /*                                */},
/*          | previous      */   { &awkawk::do_previous   /* unloaded << stopped << playing */},
/*          | next          */   { &awkawk::do_next       /* unloaded << stopped << playing */},
/*          | rwnd          */   { nullptr                /*                                */},
/*          | ffwd          */   { nullptr                /*                                */}
                          },
                          {
/* playing  | load          */   { nullptr                /*                                */},
/*          | stop          */   { &awkawk::do_stop       /* stopped                        */},
/*          | pause         */   { &awkawk::do_pause      /* paused                         */},
/*          | play          */   { &awkawk::do_pause      /* paused                         */},
/*          | unload        */   { nullptr                /*                                */},
/*          | transitioning */   { &awkawk::do_transition /* unloaded << stopped << playing */},
/*          | previous      */   { &awkawk::do_previous   /* unloaded << stopped << playing */},
/*          | next          */   { &awkawk::do_next       /* unloaded << stopped << playing */},
/*          | rwnd          */   { &awkawk::do_rwnd       /* playing                        */},
/*          | ffwd          */   { &awkawk::do_ffwd       /* playing                        */}
                          }
};

awkawk::awkawk() : ui(new player_window(this)),
                   plist(new player_playlist()),
                   current_state(unloaded),
                   handler(new event_handler_type(this)),
                   fullscreen(false),
                   chosen_ar(0),
                   chosen_lb(0),
                   wnd_size_mode(one_hundred_percent),
                   player_cs("awkawk") {
	available_ratios.push_back(std::shared_ptr<aspect_ratio>(new natural_aspect_ratio(this)));
	available_ratios.push_back(std::shared_ptr<aspect_ratio>(new fixed_aspect_ratio(4, 3)));
	available_ratios.push_back(std::shared_ptr<aspect_ratio>(new fixed_aspect_ratio(14, 9)));
	available_ratios.push_back(std::shared_ptr<aspect_ratio>(new fixed_aspect_ratio(16, 9)));
	available_ratios.push_back(std::shared_ptr<aspect_ratio>(new fixed_aspect_ratio(185, 100, L"1.85:1")));
	available_ratios.push_back(std::shared_ptr<aspect_ratio>(new fixed_aspect_ratio(240, 100, L"2.40:1")));

	available_letterboxes.push_back(std::shared_ptr<letterbox>(new natural_letterbox(this)));
	available_letterboxes.push_back(std::shared_ptr<letterbox>(new fixed_letterbox(4, 3)));
	available_letterboxes.push_back(std::shared_ptr<letterbox>(new fixed_letterbox(14, 9)));
	available_letterboxes.push_back(std::shared_ptr<letterbox>(new fixed_letterbox(16, 9)));
	available_letterboxes.push_back(std::shared_ptr<letterbox>(new fixed_letterbox(185, 100, L"1.85:1")));
	available_letterboxes.push_back(std::shared_ptr<letterbox>(new fixed_letterbox(240, 100, L"2.40:1")));

	SIZE sz = { 640, 480 };
	window_size = sz;
	video_size = sz;
	scene_size = sz;
}

awkawk::~awkawk() {
	// ensure this gets torn down first, regardless of declaration order
	renderer->stop_rendering();
	scene.reset();
	dshow.reset();
}

void awkawk::create_ui(int cmd_show) {
	ui->create_window(cmd_show);
	texture_queue.reset(new texture_queue_type());
	renderer.reset(new d3d_renderer      (this,              texture_queue.get(),                 get_ui()->get_window()                                                 ));
	dshow.reset   (new player_direct_show(this,              texture_queue.get(), renderer.get(), get_ui()->get_window()                                                 ));
	scene.reset   (new player_scene      (this, dshow.get(), texture_queue.get(),                 get_ui(),               dynamic_cast<direct3d_manager*>(renderer.get())));
}

int awkawk::run_ui() {
	renderer->start_rendering();
	return get_ui()->pump_messages();
}

awkawk::awkawk_state awkawk::do_load() {
	scene->set_filename(plist->get_file_name());
	dshow->post_event(player_direct_show::load);
	return stopped;
}

awkawk::awkawk_state awkawk::do_stop() {
	dshow->post_event_with_callback(player_direct_show::stop, [=] {
		get_ui()->async([=] (player_window* w) {
			w->set_on_top(false);
		});
	});

	return stopped;
}

awkawk::awkawk_state awkawk::do_pause() {
	dshow->post_event(player_direct_show::pause);
	return paused;
}

awkawk::awkawk_state awkawk::do_resume() {
	dshow->post_event(player_direct_show::pause);
	return playing;
}

awkawk::awkawk_state awkawk::do_play() {
	dshow->post_event_with_callback(player_direct_show::play, [=] {
		get_ui()->async([=] (player_window* w) {
			w->set_on_top(true);
		});
	});

	return playing;
}

awkawk::awkawk_state awkawk::do_unload() {
	scene->set_filename(L"");
	dshow->post_event(player_direct_show::unload);
	return unloaded;
}

awkawk::awkawk_state awkawk::do_track_change(void (player_playlist::*change_fn)(void)) {
	if(plist->empty()) {
		return unloaded;
	}
	awkawk_state initial_state(get_current_state());
	if(playing == initial_state || paused == initial_state) {
		post_event(stop);
	}
	if(unloaded != initial_state) {
		post_event(unload);
	}
	post_callback([=] {
		(*plist.*change_fn)();
		if(plist->after_end()) {
			return;
		}
		post_event(load);
		if(initial_state == playing) {
			post_event(play);
		}
	});
	return static_cast<awkawk_state>(-1);
}

awkawk::awkawk_state awkawk::do_transition() {
	return do_track_change(&player_playlist::do_transition);
}

awkawk::awkawk_state awkawk::do_previous() {
	return do_track_change(&player_playlist::do_previous);
}

awkawk::awkawk_state awkawk::do_next() {
	return do_track_change(&player_playlist::do_next);
}

awkawk::awkawk_state awkawk::do_rwnd() {
	dshow->post_event(player_direct_show::rwnd);
	return playing;
}

awkawk::awkawk_state awkawk::do_ffwd() {
	dshow->post_event(player_direct_show::ffwd);
	return playing;
}

void awkawk::set_linear_volume(float vol) {
	dshow->set_linear_volume(vol);
}

void awkawk::set_playback_position(float pos) {
	dshow->set_playback_position(pos);
}

std::vector<IBaseFilterPtr> awkawk::get_filters() const {
	return dshow->get_filters();
}

void awkawk::apply_sizing_policy() {
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

	if(is_fullscreen()) {
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
	} else {
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

	if(scene.get() != nullptr) {
		scene->notify_window_size_change();
	}
}
