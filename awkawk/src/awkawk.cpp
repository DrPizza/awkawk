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
                   player_cs("awkawk")
{
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

awkawk::~awkawk()
{
	// ensure this gets torn down first, regardless of declaration order
	renderer->stop_rendering();
	scene.reset();
	dshow.reset();
}

void awkawk::create_ui(int cmd_show)
{
	ui->create_window(cmd_show);
	texture_queue.reset(new shared_texture_queue());
	renderer.reset(new d3d_renderer      (this,              texture_queue.get(),                 get_ui()->get_window()                                                 ));
	dshow.reset   (new player_direct_show(this,              texture_queue.get(), renderer.get(), get_ui()->get_window()                                                 ));
	scene.reset   (new player_scene      (this, dshow.get(), texture_queue.get(),                 get_ui(),               dynamic_cast<direct3d_manager*>(renderer.get())));
}

int awkawk::run_ui()
{
	renderer->start_rendering();
	return get_ui()->pump_messages();
}

awkawk::awkawk_state awkawk::do_load()
{
	scene->set_filename(plist->get_file_name());
	dshow->post_event(player_direct_show::load);
	return stopped;
}

awkawk::awkawk_state awkawk::do_stop()
{
	dshow->post_event_with_callback(player_direct_show::stop, [=] {
		get_ui()->async([=] (player_window* w) {
			w->set_on_top(false);
		});
	});

	return stopped;
}

awkawk::awkawk_state awkawk::do_pause()
{
	dshow->post_event(player_direct_show::pause);
	return paused;
}

awkawk::awkawk_state awkawk::do_resume()
{
	dshow->post_event(player_direct_show::pause);
	return playing;
}

awkawk::awkawk_state awkawk::do_play()
{
	dshow->post_event_with_callback(player_direct_show::play, [=] {
		get_ui()->async([=] (player_window* w) {
			w->set_on_top(true);
		});
	});

	return playing;
}

awkawk::awkawk_state awkawk::do_unload()
{
	scene->set_filename(L"");
	dshow->post_event(player_direct_show::unload);
	return unloaded;
}

awkawk::awkawk_state awkawk::do_track_change(void (player_playlist::*change_fn)(void))
{
	if(plist->empty())
	{
		return unloaded;
	}
	awkawk_state initial_state(get_current_state());
	if(stopped != initial_state)
	{
		post_event(stop);
	}
	if(unloaded != initial_state)
	{
		post_event(unload);
	}
	post_callback([=] {
		(*plist.*change_fn)();
		if(plist->after_end())
		{
			return;
		}
		post_event(load);
		if(initial_state == playing)
		{
			post_event(play);
		}
	});
	return static_cast<awkawk_state>(-1);
}

awkawk::awkawk_state awkawk::do_transition()
{
	return do_track_change(&player_playlist::do_transition);
}

awkawk::awkawk_state awkawk::do_previous()
{
	return do_track_change(&player_playlist::do_previous);
}

awkawk::awkawk_state awkawk::do_next()
{
	return do_track_change(&player_playlist::do_next);
}

awkawk::awkawk_state awkawk::do_rwnd()
{
	dshow->post_event(player_direct_show::rwnd);
	return playing;
}

awkawk::awkawk_state awkawk::do_ffwd()
{
	dshow->post_event(player_direct_show::ffwd);
	return playing;
}

void awkawk::set_linear_volume(float vol)
{
	dshow->set_linear_volume(vol);
}

void awkawk::set_playback_position(float pos)
{
	dshow->set_playback_position(pos);
}

std::vector<ATL::CAdapt<IBaseFilterPtr> > awkawk::get_filters() const
{
	return dshow->get_filters();
}
