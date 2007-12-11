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

#ifndef PLAYER_SCENE__H
#define PLAYER_SCENE__H

#include "stdafx.h"
#include "strip.h"
#include "window.h"
#include "components.h"

#include "player_controls.h"

_COM_SMARTPTR_TYPEDEF(ID3DXFont, IID_ID3DXFont);
_COM_SMARTPTR_TYPEDEF(ID3DXMesh, IID_ID3DXMesh);

struct awkawk;

struct player_scene : direct3d_object, component_owner, boost::noncopyable
{
	player_scene(awkawk* player_, window* parent_);
	virtual ~player_scene();
	void render();

	void add_components(layout* lay)
	{
		controls->add_components(lay);
	}

	// create D3DPOOL_MANAGED resources
	virtual HRESULT on_device_created(IDirect3DDevice9Ptr new_device)
	{
		device = new_device;
		FAIL_RET(controls->on_device_created(device));
		FAIL_RET(video->on_device_created(device));
		return S_OK;
	}
	// create D3DPOOL_DEFAULT resources
	virtual HRESULT on_device_reset()
	{
		default_video_texture = load_texture_from_resource(device, IDR_BACKGROUND, &default_video_texture_info);
		FAIL_RET(controls->on_device_reset());
		FAIL_RET(video->on_device_reset());
		return S_OK;
	}
	// destroy D3DPOOL_DEFAULT resources
	virtual HRESULT on_device_lost()
	{
		FAIL_RET(controls->on_device_lost());
		FAIL_RET(video->on_device_lost());
		default_video_texture = NULL;
		return S_OK;
	}
	// destroy D3DPOOL_MANAGED resources
	virtual void on_device_destroyed()
	{
		video->on_device_destroyed();
		controls->on_device_destroyed();
		device = NULL;
	}

	void set_cursor_position(POINT pt)
	{
		LOCK(cs);
		controls->set_cursor_position(pt);
	}

	void set_filename(const std::wstring& name)
	{
		LOCK(cs);
		controls->set_filename(name);
	}

	void set_video_texture(IDirect3DTexture9Ptr video_texture_)
	{
		LOCK(cs);
		video_texture = video_texture_;
	}

	void notify_window_size_change()
	{
		controls->notify_window_size_change();
	}

	void set_volume(float vol_)
	{
		controls->set_volume(vol_);
	}

	void set_playback_position(float pos_)
	{
		controls->set_playback_position(pos_);
	}

private:
	void calculate_positions();
	void calculate_colours();

	utility::critical_section cs;

	IDirect3DDevice9Ptr device;

	std::auto_ptr<strip> video;

	IDirect3DTexture9Ptr default_video_texture;
	D3DXIMAGE_INFO default_video_texture_info;
	IDirect3DTexture9Ptr video_texture;

	awkawk* player;
	std::auto_ptr<player_controls> controls;
};

#endif
