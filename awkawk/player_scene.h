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

#ifndef PLAYER_SCENE__H
#define PLAYER_SCENE__H

#pragma once

#include "stdafx.h"
#include "strip.h"
#include "window.h"

#include "player_controls.h"

_COM_SMARTPTR_TYPEDEF(ID3DXFont, IID_ID3DXFont);
_COM_SMARTPTR_TYPEDEF(ID3DXMesh, IID_ID3DXMesh);

struct Player;

struct player_scene : device_loss_handler, message_handler
{
	player_scene(Player* player_, window* parent_, IDirect3DDevice9Ptr device_);
	virtual ~player_scene();
	void render();

	void begin_device_loss();
	void end_device_loss(IDirect3DDevice9Ptr device);

	bool handles_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		return controls.handles_message(wnd, message, wParam, lParam);
	}

	virtual LRESULT CALLBACK message_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		return controls.message_proc(wnd, message, wParam, lParam);
	}

	void set_cursor_position(POINT pt)
	{
		critical_section::lock l(cs);
		controls.set_cursor_position(pt);
	}

	void set_filename(const std::wstring& name)
	{
		critical_section::lock l(cs);
		controls.set_filename(name);
	}

	void set_video_texture(IDirect3DTexture9Ptr video_texture_)
	{
		critical_section::lock l(cs);
		video_texture = video_texture_;
	}

	void notify_window_size_change()
	{
		controls.notify_window_size_change();
	}

	void set_volume(float vol_)
	{
		controls.set_volume(vol_);
	}

	void set_playback_position(float pos_)
	{
		controls.set_playback_position(pos_);
	}

private:
	player_scene(const player_scene&);

	void initialize();
	void calculate_positions();

	critical_section cs;

	IDirect3DDevice9Ptr device;

	std::auto_ptr<strip> video;

	IDirect3DTexture9Ptr default_video_texture;
	D3DXIMAGE_INFO default_video_texture_info;
	IDirect3DTexture9Ptr video_texture;

	Player* player;
	player_controls controls;
};

#endif
