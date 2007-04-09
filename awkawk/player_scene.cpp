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

player_scene::player_scene(Player* player_, window* parent_) : message_handler(parent_),
                                                               player(player_),
                                                               controls(new player_controls(player, parent)),
                                                               video(new strip(4)),
                                                               video_texture(NULL)
{
}

player_scene::~player_scene()
{
}

void player_scene::render()
{
	if(device == NULL)
	{
		_com_raise_error(D3DERR_INVALIDCALL);
	}
	try
	{
		LOCK(cs);

		SIZE scene_size(player->get_scene_dimensions());
		float dx((-static_cast<float>(scene_size.cx) / 2.0f));
		float dy((-static_cast<float>(scene_size.cy) / 2.0f));

		D3DXMATRIX translation;
		D3DXMatrixTranslation(&translation, dx, dy, 0.0f);
		FAIL_THROW(device->SetTransform(D3DTS_WORLD, &translation));

		calculate_positions();
		calculate_colours();

		video->copy_to_buffer();
		// TODO resize the video texture using bicubic resize, possibly with pixel shaders?
		video->draw(video_texture != NULL ? video_texture : default_video_texture);
		controls->render();
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
}

void player_scene::calculate_positions()
{
	LOCK(cs);
	SIZE scene_size(player->get_scene_dimensions());

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

	video->vertices[0].position = strip::vertex::position3(0.0f                             , 0.0f                             , 0.0f);
	video->vertices[1].position = strip::vertex::position3(0.0f                             , static_cast<float>(scene_size.cy), 0.0f);
	video->vertices[2].position = strip::vertex::position3(static_cast<float>(scene_size.cx), 0.0f                             , 0.0f);
	video->vertices[3].position = strip::vertex::position3(static_cast<float>(scene_size.cx), static_cast<float>(scene_size.cy), 0.0f);

	video->vertices[0].tu = 0.0f; video->vertices[0].tv = 1.0f;
	video->vertices[1].tu = 0.0f; video->vertices[1].tv = 0.0f;
	video->vertices[2].tu = 1.0f; video->vertices[2].tv = 1.0f;
	video->vertices[3].tu = 1.0f; video->vertices[3].tv = 0.0f;
}

void player_scene::calculate_colours()
{
	video->vertices[0].diffuse = D3DCOLOR_ARGB(0xff, 0xff, 0xff, 0xff);
	video->vertices[1].diffuse = D3DCOLOR_ARGB(0xff, 0xff, 0xff, 0xff);
	video->vertices[2].diffuse = D3DCOLOR_ARGB(0xff, 0xff, 0xff, 0xff);
	video->vertices[3].diffuse = D3DCOLOR_ARGB(0xff, 0xff, 0xff, 0xff);
}
