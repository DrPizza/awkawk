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

#ifndef PLAYER_CONTROLS__H
#define PLAYER_CONTROLS__H

#pragma once

#include "stdafx.h"
#include "strip.h"
#include "util.h"
#include "window.h"
#include "resource.h"

_COM_SMARTPTR_TYPEDEF(ID3DXFont, IID_ID3DXFont);
_COM_SMARTPTR_TYPEDEF(ID3DXMesh, IID_ID3DXMesh);
_COM_SMARTPTR_TYPEDEF(ID3DXBuffer, IID_ID3DXBuffer);

struct Player;

struct player_controls : message_handler, direct3d_object
{
	player_controls(Player* player_, window* parent_);

	bool handles_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam);

	virtual LRESULT CALLBACK message_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam);
	void onLeftButtonDown(HWND wnd, BOOL doubleClick, int x, int y, UINT keyFlags);
	void onLeftButtonUp(HWND wnd, int x, int y, UINT keyFlags);
	void onMouseMove(HWND wnd, int x, int y, UINT keyFlags);

	bool hit_test_position_tracker(int x, int y) const;
	bool hit_test_position_slider(int x, int y) const;
	bool hit_test_volume_slider(int x, int y) const;


	enum control_images
	{
		normal = 0,
		play = 1,
		pause = 2,
		stop = 3,
		rwd = 4,
		ffwd = 5,
		volume = 6,
		timer = 7,
		screen_mode = 8,
		unknown1 = 9,
		unknown2 = 10,
		open = 11,
		control_max
	};

	enum caption_buttons
	{
		title_bar = 0,
		system_menu = 1,
		minimize = 2,
		maximize = 3,
		close = 4,
		caption_max
	};

	enum horizontal_alignment
	{
		left,
		centre,
		right
	};

	struct skin_definition
	{
		struct caption_definition
		{
			// caption arranged thus:
			//
			// [padding] system menu [padding] title [padding] window controls [padding]
			//                              (stretched)
			float left_padding;
			float system_menu_width;
			float system_title_padding;
			float title_window_padding;
			float window_control_width;
			float right_padding;
		};
		struct controls_definition
		{
			// controls arranged thus:
			//
			// [padding] buttons [padding] position slider [padding] volume slider [padding]
			//                               (stretched)
			float left_padding;
			float button_bar_width;
			float button_position_padding;
			float position_volume_padding;
			float volume_bar_width;
			float right_padding;
		};
		struct rectangular_button_definition
		{
			RECT rect;
			caption_buttons value;
			horizontal_alignment alignment;
		};
		struct circular_button_definition
		{
			POINT centre;
			float radius;
			control_images value;
		};
		struct position_slider_definition
		{
			float bottom_padding;
		};
		struct volume_slider_definition
		{
			float bottom_padding;
			float slider_height;
		};

		caption_definition caption;
		rectangular_button_definition caption_button_definitions[caption_max];
		controls_definition controls;
		circular_button_definition control_definitions[control_max];
		position_slider_definition position_slider;
		volume_slider_definition volume_slider;
	};

	control_images hit_test_controls(int x, int y);

	control_images get_current_control(int x, int y);

	caption_buttons hit_test_caption(int x, int y);

	void render();

	// create D3DPOOL_MANAGED resources
	virtual HRESULT on_device_created(IDirect3DDevice9Ptr new_device)
	{
		device = new_device;

		controls->on_device_created(device);
		trackbar->on_device_created(device);
		position_tracker->on_device_created(device);
		shadowed_position_tracker->on_device_created(device);
		volume_tracker->on_device_created(device);
		caption->on_device_created(device);

		return S_OK;
	}
	// create D3DPOOL_DEFAULT resources
	virtual HRESULT on_device_reset()
	{
		controls_texture = load_texture_from_resource(device, IDR_BACKGROUND_CONTROLS, &controls_texture_info);
		trackbar_texture = load_texture_from_resource(device, IDR_TRACKBAR, &trackbar_texture_info);
		position_tracker_texture = load_texture_from_resource(device, IDR_TRACKBAR_TIP, &position_tracker_texture_info);
		shadowed_position_tracker_texture = load_texture_from_resource(device, IDR_TRACKBAR_TIP, &shadowed_position_tracker_texture_info);
		volume_tracker_texture = load_texture_from_resource(device, IDR_VOLUME_TIP, &volume_tracker_texture_info);
		caption_texture = load_texture_from_resource(device, IDR_BACKGROUND_CAPTION, &caption_texture_info);

		controls->on_device_reset();
		trackbar->on_device_reset();
		position_tracker->on_device_reset();
		shadowed_position_tracker->on_device_reset();
		volume_tracker->on_device_reset();
		caption->on_device_reset();

		return S_OK;
	}
	// destroy D3DPOOL_DEFAULT resources
	virtual HRESULT on_device_lost()
	{
		controls->on_device_lost();
		trackbar->on_device_lost();
		position_tracker->on_device_lost();
		shadowed_position_tracker->on_device_lost();
		volume_tracker->on_device_lost();
		caption->on_device_lost();

		controls_texture = NULL;
		trackbar_texture = NULL;
		position_tracker_texture = NULL;
		shadowed_position_tracker_texture = NULL;
		volume_tracker_texture = NULL;
		caption_texture = NULL;
		return S_OK;
	}
	// destroy D3DPOOL_DEFAULT resources
	virtual void on_device_destroyed()
	{
		controls->on_device_destroyed();
		trackbar->on_device_destroyed();
		position_tracker->on_device_destroyed();
		shadowed_position_tracker->on_device_destroyed();
		volume_tracker->on_device_destroyed();
		caption->on_device_destroyed();
		caption_mesh = NULL;

		device = NULL;
	}

	void notify_window_size_change()
	{
		set_compact_filename();
	}

	void set_cursor_position(POINT pt)
	{
		critical_section::lock l(cs);
		cursor_position.x = pt.x;
		cursor_position.y = pt.y;
	}

	void set_filename(const std::wstring& name);

	void set_volume(float vol_)
	{
		vol = vol_;
	}

	void set_playback_position(float pos_)
	{
		playback_pos = pos_;
	}

private:
	void set_compact_filename();

	void calculate_positions();

	void calculate_caption();

	critical_section cs;

	IDirect3DDevice9Ptr device;

	std::auto_ptr<strip> controls;
	IDirect3DTexture9Ptr controls_texture;
	D3DXIMAGE_INFO controls_texture_info;

	std::auto_ptr<strip> trackbar;
	IDirect3DTexture9Ptr trackbar_texture;
	D3DXIMAGE_INFO trackbar_texture_info;

	std::auto_ptr<strip> position_tracker;
	IDirect3DTexture9Ptr position_tracker_texture;
	D3DXIMAGE_INFO position_tracker_texture_info;

	std::auto_ptr<strip> shadowed_position_tracker;
	IDirect3DTexture9Ptr shadowed_position_tracker_texture;
	D3DXIMAGE_INFO shadowed_position_tracker_texture_info;

	std::auto_ptr<strip> volume_tracker;
	IDirect3DTexture9Ptr volume_tracker_texture;
	D3DXIMAGE_INFO volume_tracker_texture_info;

	std::auto_ptr<strip> caption;
	IDirect3DTexture9Ptr caption_texture;
	D3DXIMAGE_INFO caption_texture_info;

	skin_definition skin;

	POINT cursor_position;
	float ui_reveal_percentage;
	control_images chosen_image;

	float text_size;
	std::wstring caption_text;
	std::wstring compact_caption_text;
	ID3DXMeshPtr caption_mesh;
	D3DXMATRIX caption_transform;
	SIZE caption_dimensions;

	float vol;
	float playback_pos;

	// I really ought to make these draggable things into a class or something, because there's a lot of duplication between the 
	// position handle and the volume handle
	bool dragging_position_tracker;
	int position_drag_offset;
	float position_drag_percentage;
	float position_tracker_midpoint;

	bool clicking_volume_slider;

	Player* player;

	player_controls(const player_controls&);
};

#endif
