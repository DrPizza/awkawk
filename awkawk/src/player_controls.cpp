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

#include "player_controls.h"
#include "awkawk.h"
#include "player_direct_show.h"

player_controls::player_controls(awkawk* player_,
                                 direct3d_manager* manager_,
                                 window* parent_) : direct3d_renderable(manager_),
                                                    control(parent_),
                                                    ui_reveal_percentage(0.0f),
                                                    text_size(14.0f),
                                                    caption_text(L"<(@) awkawk (@)>"),
                                                    chosen_image(normal),
                                                    dragging_position_tracker(false),
                                                    position_drag_percentage(0.0f),
                                                    clicking_volume_slider(false),
                                                    controls(new strip(8, manager_)),
                                                    trackbar(new strip(8, manager_)),
                                                    position_tracker(new strip(4, manager_)),
                                                    shadowed_position_tracker(new strip(4, manager_)),
                                                    volume_tracker(new strip(4, manager_)),
                                                    caption(new strip(8, manager_)),
                                                    overlay(new player_overlay(player_, manager_)),
                                                    player(player_),
                                                    cs("player_controls") {
	// TODO Ultimately I'd read these hardcoded sizes out of a "skin configuration" kind of a file
	skin_definition sd = { { 0.0f, 12.0f, 2.0f, 2.0f, 30.0f, 0.0f },
	                       {
	                           { {  0, 0,   0,  0}, title_bar  , left },
	                           { {  0, 0,  16, 16}, system_menu, left },
	                           { {-28, 0, -19,  9}, minimize   , right},
	                           { {-19, 0, -10,  9}, maximize   , right},
	                           { {-10, 0,   0,  9}, close      , right},
	                       },
	                       { 0.0f, 94.0f, 4.0f, 4.0f, 24.0f, 4.0f },
	                       {
	                           { {  0,  0}, 0.0f, normal      },
	                           { { 14, 13}, 7.5f, play        },
	                           { { 31, 13}, 7.5f, pause       },
	                           { { 48, 13}, 7.5f, stop        },
	                           { { 65, 13}, 7.5f, rwd         },
	                           { { 82, 13}, 7.5f, ffwd        },
	                           { { 14, 30}, 7.5f, timer       },
	                           { { 31, 30}, 7.5f, screen_mode },
	                           { { 48, 30}, 7.5f, unknown1    },
	                           { { 65, 30}, 7.5f, unknown2    },
	                           { { 82, 30}, 7.5f, open        }
	                       },
	                       { 12.0f },
	                       { 4.0f, 28.0f }
	};

	skin = sd;
}

LRESULT CALLBACK player_controls::message_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = false;
	// if something has it captured and it isn't me, don't interfere (or else things can forget to release the capture)
	if(NULL != ::GetCapture() && (!dragging_position_tracker && !clicking_volume_slider))
	{
		return 0;
	}
	switch(message)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONUP:
		{
			POINT pt = { GET_X_LPARAM(::GetMessagePos()), GET_Y_LPARAM(::GetMessagePos()) };
			RECT window_rect(get_owning_window()->get_window_rect());
			pt.x -= window_rect.left;
			pt.y -= window_rect.top;
			handled = (hit_test_caption(pt.x, pt.y) != title_bar)
			       || (hit_test_controls(pt.x, pt.y) != normal)
			       || (hit_test_position_slider(pt.x, pt.y))
			       || (hit_test_position_tracker(pt.x, pt.y))
			       || (hit_test_volume_slider(pt.x, pt.y))
			       || dragging_position_tracker
			       || clicking_volume_slider;
		}
	}
	switch(message)
	{
	case WM_LBUTTONDOWN:
		return HANDLE_WM_LBUTTONDOWN(window, wParam, lParam, onLeftButtonDown);
	case WM_LBUTTONDBLCLK:
		return HANDLE_WM_LBUTTONDBLCLK(window, wParam, lParam, onLeftButtonDown);
	case WM_LBUTTONUP:
		return HANDLE_WM_LBUTTONUP(window, wParam, lParam, onLeftButtonUp);
	case WM_MOUSEMOVE:
		if(dragging_position_tracker || clicking_volume_slider) {
			handled = true;
			return HANDLE_WM_MOUSEMOVE(window, wParam, lParam, onMouseMove);
		}
	}
	return 0;
}

void player_controls::onLeftButtonDown(HWND, BOOL, int x, int y, UINT)
{
	if(hit_test_position_slider(x, y))
	{
		dragging_position_tracker = true;
		position_drag_offset = x - static_cast<int>(98.0f + (19.0f / 2.0f));
		::SetCapture(get_owning_window()->get_window());
		return;
	}
	else if(hit_test_volume_slider(x, y))
	{
		// the volume tracker is more convenient than the position tracker, because its effect is instantaneous
		// (rather than on mouseup), so I don't need to maintain a ghost tracker or anything like that.  I can 
		// just change the volume; the renderloop queries for the volume each iteration, so it will just draw 
		// in the right place.
		clicking_volume_slider = true;
		RECT window_rect(get_owning_window()->get_window_rect());
		float new_vol(25.0f - static_cast<float>(y - ((window_rect.bottom - window_rect.top) - static_cast<LONG>(controls->vertices[1].position.y) + 8)));
		new_vol /= 25.0f;
		new_vol = clamp(new_vol, 0.0f, 1.0f);
		player->set_linear_volume(new_vol);
		::SetCapture(get_owning_window()->get_window());
		return;
	}
}

void player_controls::onLeftButtonUp(HWND, int x, int y, UINT)
{
	RECT window_rect(get_owning_window()->get_window_rect());
	if(dragging_position_tracker)
	{
		dragging_position_tracker = false;
		::ReleaseCapture();
		player->set_playback_position(position_drag_percentage);
		return;
	}
	else if(clicking_volume_slider)
	{
		clicking_volume_slider = false;
		::ReleaseCapture();
		return;
	}
	else if(caption_buttons chosen = hit_test_caption(x, y))
	{
		switch(chosen)
		{
		case system_menu:
			{
				get_owning_window()->post_message(WM_SYSMENU, 0, MAKELPARAM(window_rect.left + x, window_rect.top + y));
			}
			return;
		case minimize:
			get_owning_window()->post_message(WM_SYSCOMMAND, SC_MINIMIZE, 0);
			return;
		case maximize:
			if(player->is_fullscreen())
			{
				get_owning_window()->post_message(WM_SYSCOMMAND, SC_RESTORE, 0);
			}
			else
			{
				get_owning_window()->post_message(WM_SYSCOMMAND, SC_MAXIMIZE, 0);
			}
			return;
		case close:
			get_owning_window()->post_message(WM_SYSCOMMAND, SC_CLOSE, 0);
			return;
		}
	}
	else if(control_images chosen = hit_test_controls(x, y))
	{
		switch(chosen)
		{
		case play:
			get_owning_window()->post_message(WM_COMMAND, MAKEWPARAM(IDM_PLAY, 1), 0);
			return;
		case pause:
			get_owning_window()->post_message(WM_COMMAND, MAKEWPARAM(IDM_PAUSE, 1), 0);
			return;
		case stop:
			get_owning_window()->post_message(WM_COMMAND, MAKEWPARAM(IDM_STOP, 1), 0);
			return;
		case rwd:
			get_owning_window()->post_message(WM_COMMAND, MAKEWPARAM(IDM_PREV, 1), 0);
			return;
		case ffwd:
			get_owning_window()->post_message(WM_COMMAND, MAKEWPARAM(IDM_NEXT, 1), 0);
			return;
		case volume:
			break;
		case timer:
			break;
		case screen_mode:
			if(player->is_fullscreen())
			{
				get_owning_window()->post_message(WM_SYSCOMMAND, SC_RESTORE, 0);
			}
			else
			{
				get_owning_window()->post_message(WM_SYSCOMMAND, SC_MAXIMIZE, 0);
			}
			return;
		case unknown1:
			break;
		case unknown2:
			break;
		case open:
			get_owning_window()->post_message(WM_COMMAND, MAKEWPARAM(IDM_OPEN_FILE, 1), 0);
			return;
		}
	}
}

player_controls::control_images player_controls::get_current_control(int x, int y) const
{
	if(control_images chosen = hit_test_controls(x, y))
	{
		return chosen;
	}
	else if(hit_test_position_tracker(x, y))
	{
		return normal;
	}
	else if(hit_test_volume_slider(x, y))
	{
		return volume;
	}
	return normal;
}

void player_controls::onMouseMove(HWND, int x, int y, UINT)
{
	if(dragging_position_tracker)
	{
		position_drag_offset = x - static_cast<int>(98.0f + (19.0f / 2.0f));
	}
	else if(clicking_volume_slider)
	{
		RECT window_rect(get_owning_window()->get_window_rect());
		float new_vol(25.0f - static_cast<float>(y - ((window_rect.bottom - window_rect.top) - static_cast<LONG>(controls->vertices[1].position.y) + 8)));
		new_vol /= 25.0f;
		player->set_linear_volume(new_vol);
	}
}

bool player_controls::hit_test_position_slider(int x, int y) const
{
	RECT window_rect(get_owning_window()->get_window_rect());
	RECT position_rect = {0};
	position_rect.left   = static_cast<LONG>(controls->vertices[3].position.x);
	position_rect.right  = static_cast<LONG>(controls->vertices[5].position.x);
	position_rect.top    = (window_rect.bottom - window_rect.top) - static_cast<LONG>(controls->vertices[3].position.y);
	position_rect.bottom = (window_rect.bottom - window_rect.top) - static_cast<LONG>(controls->vertices[2].position.y);
	POINT pt = { x, y };
	return ::PtInRect(&position_rect, pt) == TRUE;
}

bool player_controls::hit_test_position_tracker(int x, int y) const
{
	RECT window_rect(get_owning_window()->get_window_rect());
	RECT tracker_rect = {0};
	tracker_rect.left   = static_cast<LONG>(position_tracker->vertices[1].position.x);
	tracker_rect.top    = (window_rect.bottom - window_rect.top) - static_cast<LONG>(position_tracker->vertices[1].position.y);
	tracker_rect.right  = static_cast<LONG>(position_tracker->vertices[2].position.x);
	tracker_rect.bottom = (window_rect.bottom - window_rect.top) - static_cast<LONG>(position_tracker->vertices[2].position.y);
	tracker_rect = normalize(tracker_rect);
	POINT pt = { x, y };
	return ::PtInRect(&tracker_rect, pt) == TRUE;
}

bool player_controls::hit_test_volume_slider(int x, int y) const
{
	RECT window_rect(get_owning_window()->get_window_rect());
	RECT volume_rect = {0};
	volume_rect.left   = static_cast<LONG>(controls->vertices[6].position.x) - 25;
	volume_rect.right  = static_cast<LONG>(controls->vertices[6].position.x);
	volume_rect.top    = (window_rect.bottom - window_rect.top) - static_cast<LONG>(controls->vertices[1].position.y) + 8;
	volume_rect.bottom = (window_rect.bottom - window_rect.top) - static_cast<LONG>(controls->vertices[6].position.y) - 5;
	POINT pt = { x, y };
	return ::PtInRect(&volume_rect, pt) == TRUE;
}

// two rows of five circles of diameter 15.  Centres, relative to the top left of the texture are at:
// (14, 13) (31, 13) (48, 13) (65, 13) (82, 13)
// (14, 30) (31, 30) (48, 30) (65, 30) (82, 30)
// and they are:
// play pause stop |< >|
// timer switch-size unknown1 unknown2 open
// then at the right hand side is a 25x40 panel for the volume slider
player_controls::control_images player_controls::hit_test_controls(int x, int y) const
{
	POINT pt = { x, y };
	RECT window_rect(get_owning_window()->get_window_rect());
	RECT controls_rect = {0};
	controls_rect.left   = static_cast<LONG>(controls->vertices[1].position.x);
	controls_rect.top    = (window_rect.bottom - window_rect.top) - static_cast<LONG>(controls->vertices[1].position.y);
	controls_rect.right  = static_cast<LONG>(controls->vertices[6].position.x);
	controls_rect.bottom = (window_rect.bottom - window_rect.top) - static_cast<LONG>(controls->vertices[6].position.y);
	pt.y -= controls_rect.top;
	for(size_t i(0); i < sizeof(skin.control_definitions) / sizeof(skin.control_definitions[0]); ++i)
	{
		if(point_in_circle(pt, skin.control_definitions[i].centre, skin.control_definitions[i].radius))
		{
			return skin.control_definitions[i].value;
		}
	}
	return normal;
}

player_controls::caption_buttons player_controls::hit_test_caption(int x, int y) const
{
	POINT pt = { x, y };
	RECT window_rect(get_owning_window()->get_window_rect());
	::OffsetRect(&window_rect, window_rect.left, window_rect.top);
	RECT caption_rect = {0};
	caption_rect.left   = static_cast<LONG>(caption->vertices[1].position.x);
	caption_rect.top    = (window_rect.bottom - window_rect.top) - static_cast<LONG>(caption->vertices[1].position.y);
	caption_rect.right  = static_cast<LONG>(caption->vertices[6].position.x);
	caption_rect.bottom = (window_rect.bottom - window_rect.top) - static_cast<LONG>(caption->vertices[6].position.y);
	pt.y += caption_rect.top;
	for(size_t i(0); i < sizeof(skin.caption_button_definitions) / sizeof(skin.caption_button_definitions[0]); ++i)
	{
		RECT button_rect(skin.caption_button_definitions[i].rect);
		if(skin.caption_button_definitions[i].alignment == right)
		{
			button_rect.left += (window_rect.right - window_rect.left);
			button_rect.right += (window_rect.right - window_rect.left);
		}
		if(::PtInRect(&button_rect, pt) == TRUE)
		{
			return skin.caption_button_definitions[i].value;
		}
	}
	return title_bar;
}

void player_controls::do_emit_scene()
{
	if(device == nullptr)
	{
		_com_raise_error(D3DERR_INVALIDCALL);
	}
	try
	{
		LOCK(cs);

		SIZE window_size(player->get_window_dimensions());
		float dx((-static_cast<float>(window_size.cx) / 2.0f) - 0.0f);
		float dy((-static_cast<float>(window_size.cy) / 2.0f) - 0.0f);

		D3DXMATRIX original_translation;
		FAIL_THROW(device->GetTransform(D3DTS_WORLD, &original_translation));
		ON_BLOCK_EXIT(device->SetTransform(D3DTS_WORLD, &original_translation));

		D3DXMATRIX translation;
		D3DXMatrixTranslation(&translation, dx, dy, 0.0f);
		FAIL_THROW(device->SetTransform(D3DTS_WORLD, &translation));

		chosen_image = get_current_control(cursor_position.x, cursor_position.y);

		calculate_positions();
		calculate_colours();

		controls->copy_to_buffer();
		trackbar->copy_to_buffer();
		if(dragging_position_tracker)
		{
			shadowed_position_tracker->copy_to_buffer();
		}
		if(chosen_image == volume)
		{
			volume_tracker->copy_to_buffer();
		}
		position_tracker->copy_to_buffer();
		caption->copy_to_buffer();

		IDirect3DStateBlock9Ptr stateBlock;
		FAIL_THROW(device->CreateStateBlock(D3DSBT_ALL, &stateBlock));
		FAIL_THROW(stateBlock->Capture());
		ON_BLOCK_EXIT(stateBlock->Apply());

		FAIL_THROW(device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE));
		FAIL_THROW(device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE));
		FAIL_THROW(device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE));

		controls->draw(controls_texture);
		trackbar->draw(trackbar_texture);
		position_tracker->draw(position_tracker_texture);
		if(dragging_position_tracker)
		{
			shadowed_position_tracker->draw(shadowed_position_tracker_texture);
		}
		if(chosen_image == volume)
		{
			volume_tracker->draw(volume_tracker_texture);
		}
		caption->draw(caption_texture);
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
}

void player_controls::calculate_positions()
{
	LOCK(cs);

	SIZE window_size(player->get_window_dimensions());

	bool growing(dragging_position_tracker || (cursor_position.y != -1 && (cursor_position.y > (window_size.cy - static_cast<float>(controls_texture_info.Height) / 12.0f) || cursor_position.y < static_cast<float>(caption_texture_info.Height))));

	if(growing && ui_reveal_percentage < 1.0f)
	{
		ui_reveal_percentage += 0.1f;
	}
	if(!growing && ui_reveal_percentage > 0.0f)
	{
		ui_reveal_percentage -= 0.1f;
	}
	ui_reveal_percentage = clamp(ui_reveal_percentage, 0.0f, 1.0f);
	// sliding stuff on and off-screen at the bottom
	float bottom_offset((ui_reveal_percentage - 1.0f) * (static_cast<float>(controls_texture_info.Height) / static_cast<float>(control_max)));
	// sliding stuff on and off-screen at the top
	float top_offset((1.0f - ui_reveal_percentage) * static_cast<float>(caption_texture_info.Height));

	player_overlay::active_area active(player_overlay::none);
	switch(chosen_image) {
	case volume:
		active = player_overlay::volume;
		break;
	case play:
		active = player_overlay::play;
		break;
	case pause:
		active = player_overlay::pause;
		break;
	case stop:
		active = player_overlay::stop;
		break;
	case rwd:
		active = player_overlay::rwd;
		break;
	case ffwd:
		active = player_overlay::ffwd;
		break;
	case timer:
		active = player_overlay::timer;
		break;
	case screen_mode:
		active = player_overlay::screen_mode;
		break;
	case unknown1:
		active = player_overlay::unknown1;
		break;
	case unknown2:
		active = player_overlay::unknown1;
		break;
	case open:
		active = player_overlay::open;
		break;
	default:
		if(hit_test_position_slider(cursor_position.x, cursor_position.y))
		{
			active = player_overlay::position;
		}
	}

	RECT caption_position = { 
		static_cast<LONG>(skin.caption.system_menu_width + skin.caption.system_title_padding),
		static_cast<LONG>(-top_offset),
		static_cast<LONG>(window_size.cx - (skin.caption.title_window_padding + skin.caption.window_control_width)),
		static_cast<LONG>(caption_texture_info.Height - top_offset)
	};

	overlay->update(vol, play_time, active, caption_text, caption_position);

	// clockwise (front)
	// 1 v1               v3             v5               v7
	//    |---------------|---------------|---------------|
	//    |   \           |   \           |   \           |
	//    |    \          |    \          |    \          |
	//    |     \         |     \         |     \         |
	//    |      \        |      \        |      \        |
	//    |       \       |       \       |       \       |
	//    |        \      |        \      |        \      |
	//    |         \     |         \     |         \     |
	//    |          \    |          \    |          \    |
	//    |---------------|---------------|---------------|
	//   v0               v2              v4              v6
	//                     
	// 0                  1

	// counterclockwise (back)
	// 1 v0               v2              v4              v6
	//    |---------------|---------------|---------------|
	//    |          /    |          /    |          /    |
	//    |         /     |         /     |         /     |
	//    |        /      |        /      |        /      |
	//    |       /       |       /       |       /       |
	//    |      /        |      /        |      /        |
	//    |     /         |     /         |     /         |
	//    |    /          |    /          |    /          |
	//    |   /           |   /           |   /           |
	//    |---------------|---------------|---------------|
	//   v1               v3              v5              v7
	//                     
	// 0                  1

	// twelve images (arranged vertically)
	// each image has a width of
	// 94px 16 px 24px (total 130 px)
	// positions: 0 (94 / width) 1 - (24 / width) 1
	static const float button_bar_width(skin.controls.left_padding + skin.controls.button_bar_width);
	static const float volume_bar_width(skin.controls.volume_bar_width + skin.controls.right_padding);
	controls->vertices[0].position = strip::vertex::position3(0.0f                                                 , 0.0f + bottom_offset                                                                                , 0.0f);
	controls->vertices[1].position = strip::vertex::position3(0.0f                                                 , (static_cast<float>(controls_texture_info.Height) / static_cast<float>(control_max)) + bottom_offset, 0.0f);
	controls->vertices[2].position = strip::vertex::position3(button_bar_width                                     , 0.0f + bottom_offset                                                                                , 0.0f);
	controls->vertices[3].position = strip::vertex::position3(button_bar_width                                     , (static_cast<float>(controls_texture_info.Height) / static_cast<float>(control_max)) + bottom_offset, 0.0f);
	controls->vertices[4].position = strip::vertex::position3(static_cast<float>(window_size.cx) - volume_bar_width, 0.0f + bottom_offset                                                                                , 0.0f);
	controls->vertices[5].position = strip::vertex::position3(static_cast<float>(window_size.cx) - volume_bar_width, (static_cast<float>(controls_texture_info.Height) / static_cast<float>(control_max)) + bottom_offset, 0.0f);
	controls->vertices[6].position = strip::vertex::position3(static_cast<float>(window_size.cx)                   , 0.0f + bottom_offset                                                                                , 0.0f);
	controls->vertices[7].position = strip::vertex::position3(static_cast<float>(window_size.cx)                   , (static_cast<float>(controls_texture_info.Height) / static_cast<float>(control_max)) + bottom_offset, 0.0f);

	// positions: 0 (94 / 130) 1 - (24 / 130) 1
	controls->vertices[0].tu = 0.0f;                                                                        controls->vertices[0].tv = (chosen_image + 1) * (1.0f / static_cast<float>(control_max));
	controls->vertices[1].tu = 0.0f;                                                                        controls->vertices[1].tv = (chosen_image + 0) * (1.0f / static_cast<float>(control_max));
	controls->vertices[2].tu = button_bar_width / static_cast<float>(controls_texture_info.Width);          controls->vertices[2].tv = (chosen_image + 1) * (1.0f / static_cast<float>(control_max));
	controls->vertices[3].tu = button_bar_width / static_cast<float>(controls_texture_info.Width);          controls->vertices[3].tv = (chosen_image + 0) * (1.0f / static_cast<float>(control_max));
	controls->vertices[4].tu = 1.0f - (volume_bar_width / static_cast<float>(controls_texture_info.Width)); controls->vertices[4].tv = (chosen_image + 1) * (1.0f / static_cast<float>(control_max));
	controls->vertices[5].tu = 1.0f - (volume_bar_width / static_cast<float>(controls_texture_info.Width)); controls->vertices[5].tv = (chosen_image + 0) * (1.0f / static_cast<float>(control_max));
	controls->vertices[6].tu = 1.0f;                                                                        controls->vertices[6].tv = (chosen_image + 1) * (1.0f / static_cast<float>(control_max));
	controls->vertices[7].tu = 1.0f;                                                                        controls->vertices[7].tv = (chosen_image + 0) * (1.0f / static_cast<float>(control_max));

	// trackbar is 6px tall, and has ends of 4px (middle bit being scalable)
	// controls are 40px tall, so bottom of trackbar is 17px, top is 23px
	//                                             (controls height +/- trackbar height) / 2
	trackbar->vertices[0].position = strip::vertex::position3(button_bar_width                                                                               , (static_cast<float>((controls_texture_info.Height / static_cast<float>(control_max)) - trackbar_texture_info.Height) / 2.0f) + bottom_offset, 0.0f);
	trackbar->vertices[1].position = strip::vertex::position3(button_bar_width                                                                               , (static_cast<float>((controls_texture_info.Height / static_cast<float>(control_max)) + trackbar_texture_info.Height) / 2.0f) + bottom_offset, 0.0f);
	trackbar->vertices[2].position = strip::vertex::position3(button_bar_width + skin.controls.button_position_padding                                       , (static_cast<float>((controls_texture_info.Height / static_cast<float>(control_max)) - trackbar_texture_info.Height) / 2.0f) + bottom_offset, 0.0f);
	trackbar->vertices[3].position = strip::vertex::position3(button_bar_width + skin.controls.button_position_padding                                       , (static_cast<float>((controls_texture_info.Height / static_cast<float>(control_max)) + trackbar_texture_info.Height) / 2.0f) + bottom_offset, 0.0f);
	trackbar->vertices[4].position = strip::vertex::position3(static_cast<float>(window_size.cx) - (skin.controls.position_volume_padding + volume_bar_width), (static_cast<float>((controls_texture_info.Height / static_cast<float>(control_max)) - trackbar_texture_info.Height) / 2.0f) + bottom_offset, 0.0f);
	trackbar->vertices[5].position = strip::vertex::position3(static_cast<float>(window_size.cx) - (skin.controls.position_volume_padding + volume_bar_width), (static_cast<float>((controls_texture_info.Height / static_cast<float>(control_max)) + trackbar_texture_info.Height) / 2.0f) + bottom_offset, 0.0f);
	trackbar->vertices[6].position = strip::vertex::position3(static_cast<float>(window_size.cx) - volume_bar_width                                          , (static_cast<float>((controls_texture_info.Height / static_cast<float>(control_max)) - trackbar_texture_info.Height) / 2.0f) + bottom_offset, 0.0f);
	trackbar->vertices[7].position = strip::vertex::position3(static_cast<float>(window_size.cx) - volume_bar_width                                          , (static_cast<float>((controls_texture_info.Height / static_cast<float>(control_max)) + trackbar_texture_info.Height) / 2.0f) + bottom_offset, 0.0f);

	trackbar->vertices[0].tu = 0.0f;                                                                                             trackbar->vertices[0].tv = 1.0f;
	trackbar->vertices[1].tu = 0.0f;                                                                                             trackbar->vertices[1].tv = 0.0f;
	trackbar->vertices[2].tu = skin.controls.button_position_padding / static_cast<float>(trackbar_texture_info.Width);          trackbar->vertices[2].tv = 1.0f;
	trackbar->vertices[3].tu = skin.controls.button_position_padding / static_cast<float>(trackbar_texture_info.Width);          trackbar->vertices[3].tv = 0.0f;
	trackbar->vertices[4].tu = 1.0f - (skin.controls.position_volume_padding / static_cast<float>(trackbar_texture_info.Width)); trackbar->vertices[4].tv = 1.0f;
	trackbar->vertices[5].tu = 1.0f - (skin.controls.position_volume_padding / static_cast<float>(trackbar_texture_info.Width)); trackbar->vertices[5].tv = 0.0f;
	trackbar->vertices[6].tu = 1.0f;                                                                                             trackbar->vertices[6].tv = 1.0f;
	trackbar->vertices[7].tu = 1.0f;                                                                                             trackbar->vertices[7].tv = 0.0f;

	//
	//
	// [---------------------------------]
	//  /\                             /\
	//
	// starts with left edge aligned
	// ends with right edge aligned

	// the "useful" part of the tracker is 19 pixels
	float progress(static_cast<float>(playback_pos / 100.0));

	float trackbar_width(window_size.cx - (skin.controls.button_bar_width + skin.controls.button_position_padding) - volume_bar_width - static_cast<float>(position_tracker_texture_info.Width));
	position_tracker_midpoint = (skin.controls.button_bar_width + skin.controls.button_position_padding) + (progress * trackbar_width) + (static_cast<float>(position_tracker_texture_info.Width) / 2.0f);

	float position_left_edge (position_tracker_midpoint - (static_cast<float>(position_tracker_texture_info.Width) / 2.0f));
	float position_right_edge(position_tracker_midpoint + (static_cast<float>(position_tracker_texture_info.Width) / 2.0f));

	position_tracker->vertices[0].position = strip::vertex::position3(position_left_edge ,                                                            skin.position_slider.bottom_padding + bottom_offset, 0.0f);
	position_tracker->vertices[1].position = strip::vertex::position3(position_left_edge , static_cast<float>(position_tracker_texture_info.Height) + skin.position_slider.bottom_padding + bottom_offset, 0.0f);
	position_tracker->vertices[2].position = strip::vertex::position3(position_right_edge,                                                            skin.position_slider.bottom_padding + bottom_offset, 0.0f);
	position_tracker->vertices[3].position = strip::vertex::position3(position_right_edge, static_cast<float>(position_tracker_texture_info.Height) + skin.position_slider.bottom_padding + bottom_offset, 0.0f);

	position_tracker->vertices[0].tu = 0.0f; position_tracker->vertices[0].tv = 1.0f;
	position_tracker->vertices[1].tu = 0.0f; position_tracker->vertices[1].tv = 0.0f;
	position_tracker->vertices[2].tu = 1.0f; position_tracker->vertices[2].tv = 1.0f;
	position_tracker->vertices[3].tu = 1.0f; position_tracker->vertices[3].tv = 0.0f;

	if(dragging_position_tracker)
	{
		float shadowed_trackbar_width(window_size.cx - (skin.controls.button_bar_width + skin.controls.button_position_padding) - volume_bar_width - static_cast<float>(position_tracker_texture_info.Width));
		position_drag_offset = std::min(std::max(position_drag_offset, 0), static_cast<int>(shadowed_trackbar_width));
		float shadowed_tracker_midpoint((skin.controls.button_bar_width + skin.controls.button_position_padding) + static_cast<float>(position_drag_offset) + (static_cast<float>(position_tracker_texture_info.Width) / 2.0f));
		position_drag_percentage = (position_drag_offset / shadowed_trackbar_width) * 100.0f;

		float shadowed_left_edge (shadowed_tracker_midpoint - (static_cast<float>(shadowed_position_tracker_texture_info.Width) / 2.0f));
		float shadowed_right_edge(shadowed_tracker_midpoint + (static_cast<float>(shadowed_position_tracker_texture_info.Width) / 2.0f));
		
		shadowed_position_tracker->vertices[0].position = strip::vertex::position3(shadowed_left_edge ,                                                                     skin.position_slider.bottom_padding + bottom_offset, 0.0f);
		shadowed_position_tracker->vertices[1].position = strip::vertex::position3(shadowed_left_edge , static_cast<float>(shadowed_position_tracker_texture_info.Height) + skin.position_slider.bottom_padding + bottom_offset, 0.0f);
		shadowed_position_tracker->vertices[2].position = strip::vertex::position3(shadowed_right_edge,                                                                     skin.position_slider.bottom_padding + bottom_offset, 0.0f);
		shadowed_position_tracker->vertices[3].position = strip::vertex::position3(shadowed_right_edge, static_cast<float>(shadowed_position_tracker_texture_info.Height) + skin.position_slider.bottom_padding + bottom_offset, 0.0f);

		shadowed_position_tracker->vertices[0].tu = 0.0f; shadowed_position_tracker->vertices[0].tv = 1.0f;
		shadowed_position_tracker->vertices[1].tu = 0.0f; shadowed_position_tracker->vertices[1].tv = 0.0f;
		shadowed_position_tracker->vertices[2].tu = 1.0f; shadowed_position_tracker->vertices[2].tv = 1.0f;
		shadowed_position_tracker->vertices[3].tu = 1.0f; shadowed_position_tracker->vertices[3].tv = 0.0f;
	}

	float volume_tracker_midpoint = static_cast<float>((linear_vol * skin.volume_slider.slider_height) + skin.volume_slider.bottom_padding);

	float volume_top_edge   (volume_tracker_midpoint - (static_cast<float>(volume_tracker_texture_info.Height) / 2.0f));
	float volume_bottom_edge(volume_tracker_midpoint + (static_cast<float>(volume_tracker_texture_info.Height) / 2.0f));

	float volume_left_edge (static_cast<float>(window_size.cx) - static_cast<float>(volume_tracker_texture_info.Width) - skin.controls.right_padding);
	float volume_right_edge(static_cast<float>(window_size.cx) - skin.controls.right_padding);

	volume_tracker->vertices[0].position = strip::vertex::position3(volume_left_edge , volume_top_edge    + bottom_offset, 0.0f);
	volume_tracker->vertices[1].position = strip::vertex::position3(volume_left_edge , volume_bottom_edge + bottom_offset, 0.0f);
	volume_tracker->vertices[2].position = strip::vertex::position3(volume_right_edge, volume_top_edge    + bottom_offset, 0.0f);
	volume_tracker->vertices[3].position = strip::vertex::position3(volume_right_edge, volume_bottom_edge + bottom_offset, 0.0f);

	volume_tracker->vertices[0].tu = 0.0f; volume_tracker->vertices[1].tv = 0.0f;
	volume_tracker->vertices[1].tu = 0.0f; volume_tracker->vertices[0].tv = 1.0f;
	volume_tracker->vertices[2].tu = 1.0f; volume_tracker->vertices[3].tv = 0.0f;
	volume_tracker->vertices[3].tu = 1.0f; volume_tracker->vertices[2].tv = 1.0f;

	// caption is 16px tall, and has ends of 12px (left) and 30 px (right), total width of 60 px
	caption->vertices[0].position = strip::vertex::position3(0.0f                                                                                                 , static_cast<float>(window_size.cy) - caption_texture_info.Height + top_offset, 0.0f);
	caption->vertices[1].position = strip::vertex::position3(0.0f                                                                                                 , static_cast<float>(window_size.cy) + top_offset                              , 0.0f);
	caption->vertices[2].position = strip::vertex::position3(skin.caption.left_padding + skin.caption.system_menu_width                                           , static_cast<float>(window_size.cy) - caption_texture_info.Height + top_offset, 0.0f);
	caption->vertices[3].position = strip::vertex::position3(skin.caption.left_padding + skin.caption.system_menu_width                                           , static_cast<float>(window_size.cy) + top_offset                              , 0.0f);
	caption->vertices[4].position = strip::vertex::position3(static_cast<float>(window_size.cx) - (skin.caption.window_control_width + skin.caption.right_padding), static_cast<float>(window_size.cy) - caption_texture_info.Height + top_offset, 0.0f);
	caption->vertices[5].position = strip::vertex::position3(static_cast<float>(window_size.cx) - (skin.caption.window_control_width + skin.caption.right_padding), static_cast<float>(window_size.cy) + top_offset                              , 0.0f);
	caption->vertices[6].position = strip::vertex::position3(static_cast<float>(window_size.cx)                                                                   , static_cast<float>(window_size.cy) - caption_texture_info.Height + top_offset, 0.0f);
	caption->vertices[7].position = strip::vertex::position3(static_cast<float>(window_size.cx)                                                                   , static_cast<float>(window_size.cy) + top_offset                              , 0.0f);

	caption->vertices[0].tu = 0.0f;                                                                                                                       caption->vertices[0].tv = 1.0f;
	caption->vertices[1].tu = 0.0f;                                                                                                                       caption->vertices[1].tv = 0.0f;
	caption->vertices[2].tu = (skin.caption.left_padding + skin.caption.system_menu_width) / static_cast<float>(caption_texture_info.Width);              caption->vertices[2].tv = 1.0f;
	caption->vertices[3].tu = (skin.caption.left_padding + skin.caption.system_menu_width) / static_cast<float>(caption_texture_info.Width);              caption->vertices[3].tv = 0.0f;
	caption->vertices[4].tu = 1.0f - ((skin.caption.window_control_width + skin.caption.right_padding) / static_cast<float>(caption_texture_info.Width)); caption->vertices[4].tv = 1.0f;
	caption->vertices[5].tu = 1.0f - ((skin.caption.window_control_width + skin.caption.right_padding) / static_cast<float>(caption_texture_info.Width)); caption->vertices[5].tv = 0.0f;
	caption->vertices[6].tu = 1.0f;                                                                                                                       caption->vertices[6].tv = 1.0f;
	caption->vertices[7].tu = 1.0f;                                                                                                                       caption->vertices[7].tv = 0.0f;
}

void player_controls::calculate_colours()
{
	DWORD transparent(static_cast<DWORD>(std::min(ui_reveal_percentage, 0.9f) * 255.0f));

	controls->vertices[0].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	controls->vertices[1].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	controls->vertices[2].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	controls->vertices[3].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	controls->vertices[4].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	controls->vertices[5].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	controls->vertices[6].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	controls->vertices[7].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);

	trackbar->vertices[0].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	trackbar->vertices[1].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	trackbar->vertices[2].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	trackbar->vertices[3].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	trackbar->vertices[4].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	trackbar->vertices[5].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	trackbar->vertices[6].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	trackbar->vertices[7].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);

	position_tracker->vertices[0].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	position_tracker->vertices[1].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	position_tracker->vertices[2].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	position_tracker->vertices[3].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);

	shadowed_position_tracker->vertices[0].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	shadowed_position_tracker->vertices[1].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	shadowed_position_tracker->vertices[2].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	shadowed_position_tracker->vertices[3].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);

	volume_tracker->vertices[0].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	volume_tracker->vertices[1].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	volume_tracker->vertices[2].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	volume_tracker->vertices[3].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);

	caption->vertices[0].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	caption->vertices[1].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	caption->vertices[2].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	caption->vertices[3].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	caption->vertices[4].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	caption->vertices[5].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	caption->vertices[6].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
	caption->vertices[7].diffuse = D3DCOLOR_ARGB(transparent, 0xff, 0xff, 0xff);
}

void player_controls::set_filename(const std::wstring& name)
{
	LOCK(cs);
	if(caption_text == name)
	{
		return;
	}
	caption_text = name.size() != 0 ? name : L"<(@) awkawk (@)>";
}
