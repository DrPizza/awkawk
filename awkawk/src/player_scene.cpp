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

#include "awkawk.h"
#include "player_scene.h"
#include "player_direct_show.h"
#include "shared_texture_queue.h"

player_scene::player_scene(awkawk* player_,
                           player_direct_show* dshow_,
                           shared_texture_queue* texture_queue_,
                           window* parent_,
                           direct3d_manager* manager_) : direct3d_renderable(manager_),
                                                         player(player_),
                                                         dshow(dshow_),
                                                         texture_queue(texture_queue_),
                                                         controls(new player_controls(player, manager_, parent_)),
                                                         video(new strip(4, manager_)),
                                                         video_texture(nullptr),
                                                         cs("player_scene")
{
}

player_scene::~player_scene()
{
	if(previous_texture) {
		texture_queue->consumer_enqueue(previous_texture);
		previous_texture = nullptr;
	}
}

void player_scene::do_emit_scene()
{
	if(device == nullptr)
	{
		_com_raise_error(D3DERR_INVALIDCALL);
	}

	IDirect3DTexture9Ptr current_texture;

	if(dshow->get_graph_state() != player_direct_show::unloaded)
	{
		// this is quite a hack.
		// these values can get stale if directshow gets held up for whatever reason 
		// but doing anything else seems to be deadlock city at the moment (due to 
		// locks held by DirectShow)

		// what I should be doing is getting DirectShow to update this part, rather 
		// than having to ask it the whole time.

		//static float volume = 0.0f,
		//      linear_volume = 0.0f,
		//           position = 0.0f,
		//               time = 0.0f;

		//dshow->post_callback([&] {
		//	volume = dshow->get_volume_unsync();
		//	linear_volume = dshow->get_linear_volume_unsync();
		//	position = dshow->get_playback_position_unsync();
		//	time = dshow->get_play_time_unsync();
		//});

		//set_volume(volume);
		//set_linear_volume(linear_volume);
		//set_playback_position(position);
		//set_play_time(time);

		set_volume(dshow->get_volume_unsync());
		set_linear_volume(dshow->get_linear_volume_unsync());
		set_playback_position(dshow->get_playback_position_unsync());
		set_play_time(dshow->get_play_time_unsync());

		if(dshow->get_has_video()) {
			IDirect3DTexture9Ptr result(texture_queue->consumer_dequeue());

			if(!result) {
				current_texture = previous_texture;
			} else {
				if(previous_texture) {
					texture_queue->consumer_enqueue(previous_texture);
				}
				current_texture = result;
				previous_texture = current_texture;
			}
		} else {
			if(previous_texture) {
				texture_queue->consumer_enqueue(previous_texture);
				previous_texture = nullptr;
			}
		}
	}
	else
	{
		set_volume(0.0f);
		set_linear_volume(1.0f);
		set_playback_position(0.0f);
		set_play_time(0.0f);
		if(previous_texture) {
			texture_queue->consumer_enqueue(previous_texture);
			previous_texture = nullptr;
		}
	}
	if(current_texture) {
		//static const GUID texture_number_guid = { 0xfad31a05, 0x1099, 0x4eb5, 0x9e, 0x30, 0x71, 0x78, 0x18, 0xfa, 0xf1, 0xe2 };
		//static const GUID timestamp_guid      = { 0x6af09a08, 0x5e5b, 0x40e7, 0x99, 0x0c, 0xa4, 0x21, 0x12, 0x4b, 0x97, 0xa6 };
		//DWORD texture_count(0);
		//DWORD size(sizeof(texture_count));
		//current_texture.first->GetPrivateData(texture_number_guid, &texture_count, &size);

		//REFERENCE_TIME timestamp(0);
		//size = sizeof(timestamp);
		//current_texture.first->GetPrivateData(timestamp_guid, &timestamp, &size);

		//dout << timestamp << " " << texture_count << std::endl;
	}
	set_video_texture(current_texture);
	try
	{
		LOCK(cs);

		SIZE scene_size(player->get_scene_dimensions());
		float dx((-static_cast<float>(scene_size.cx) / 2.0f));
		float dy((-static_cast<float>(scene_size.cy) / 2.0f));

		D3DXMATRIX original_translation;
		FAIL_THROW(device->GetTransform(D3DTS_WORLD, &original_translation));
		ON_BLOCK_EXIT(device->SetTransform(D3DTS_WORLD, &original_translation));

		D3DXMATRIX translation;
		D3DXMatrixTranslation(&translation, dx, dy, 0.0f);
		FAIL_THROW(device->SetTransform(D3DTS_WORLD, &translation));

		calculate_positions();
		calculate_colours();

		video->copy_to_buffer();
		// TODO resize the video texture using bicubic resize, possibly with pixel shaders?
		video->draw(video_texture != nullptr ? video_texture : default_video_texture);
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
}

void player_scene::calculate_positions()
{
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

HRESULT player_scene::do_on_device_created(IDirect3DDevice9Ptr new_device)
{
	device = new_device;
	FAIL_RET(controls->on_device_created(device));
	FAIL_RET(video->on_device_created(device));
	texture_queue->set_consumer(device);
	return S_OK;
}
// create D3DPOOL_DEFAULT resources
HRESULT player_scene::do_on_device_reset()
{
	default_video_texture = load_texture_from_resource(device, IDR_BACKGROUND, &default_video_texture_info);
	FAIL_RET(controls->on_device_reset());
	FAIL_RET(video->on_device_reset());
	return S_OK;
}
// destroy D3DPOOL_DEFAULT resources
HRESULT player_scene::do_on_device_lost()
{
	FAIL_RET(controls->on_device_lost());
	FAIL_RET(video->on_device_lost());
	default_video_texture = nullptr;
	return S_OK;
}
// destroy D3DPOOL_MANAGED resources
void player_scene::do_on_device_destroyed()
{
	texture_queue->set_consumer(nullptr);
	video->on_device_destroyed();
	controls->on_device_destroyed();
	device = nullptr;
}
