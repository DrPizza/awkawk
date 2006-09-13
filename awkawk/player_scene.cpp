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
#include "util.h"
#include "resource.h"
#include "player.h"
#include "player_scene.h"

player_scene::player_scene(Player* player_, window* parent_, IDirect3DDevice9Ptr device_) : message_handler(parent_), player(player_), device(device_), controls(player, parent, device), video_texture(NULL)
{
	if(NULL == device)
	{
		_com_raise_error(E_POINTER);
	}
	initialize();
	video.reset(new strip(device, 4));
}

void player_scene::begin_device_loss()
{
	device = NULL;
	default_video_texture = NULL;
	controls.begin_device_loss();
	video->begin_device_loss();
}

void player_scene::end_device_loss(IDirect3DDevice9Ptr device_)
{
	device = device_;
	initialize();
	controls.end_device_loss(device);
	video->end_device_loss(device);
}

void player_scene::initialize()
{
	default_video_texture = load_texture_from_resource(device, IDR_BACKGROUND, &default_video_texture_info);
}

player_scene::~player_scene()
{
}

void player_scene::render()
{
	try
	{
		critical_section::lock l(cs);
		SIZE window_size(player->get_window_dimensions());
		SIZE scene_size(player->get_scene_dimensions());
		D3DXMATRIX ortho2D;
		D3DXMatrixOrthoLH(&ortho2D, static_cast<float>(window_size.cx), static_cast<float>(window_size.cy), 0.0f, 1.0f);
		FAIL_THROW(device->SetTransform(D3DTS_PROJECTION, &ortho2D));

		float dx((-static_cast<float>(scene_size.cx) / 2.0f) - 0.5f);
		float dy((-static_cast<float>(scene_size.cy) / 2.0f) - 0.5f);

		D3DXMATRIX translation;
		D3DXMatrixTranslation(&translation, dx, dy, 0.0f);
		FAIL_THROW(device->SetTransform(D3DTS_WORLD, &translation));

		calculate_positions();

		video->copy_to_buffer();
		// TODO resize the video texture using bicubic resize, possibly with pixel shaders?
		video->draw(video_texture != NULL ? video_texture : default_video_texture);
		controls.render();
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
}

void player_scene::calculate_positions()
{
	critical_section::lock l(cs);
	SIZE scene_size(player->get_scene_dimensions());
	video->vertices[0].position = strip::vertex::position3(0.0f                             , static_cast<float>(scene_size.cy), 0.0f);
	video->vertices[1].position = strip::vertex::position3(0.0f                             , 0.0f                             , 0.0f);
	video->vertices[2].position = strip::vertex::position3(static_cast<float>(scene_size.cx), static_cast<float>(scene_size.cy), 0.0f);
	video->vertices[3].position = strip::vertex::position3(static_cast<float>(scene_size.cx), 0.0f                             , 0.0f);

	video->vertices[0].tu = 0.0f; video->vertices[0].tv = 0.0f;
	video->vertices[1].tu = 0.0f; video->vertices[1].tv = 1.0f;
	video->vertices[2].tu = 1.0f; video->vertices[2].tv = 0.0f;
	video->vertices[3].tu = 1.0f; video->vertices[3].tv = 1.0f;
}
