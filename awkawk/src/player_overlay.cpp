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
#include "player_overlay.h"

player_overlay::player_overlay(awkawk* player_, window* parent_, direct3d_manager* manager_) : direct3d_renderable(manager_), player(player_), active(none), cs("player_overlay")
{
}

player_overlay::~player_overlay()
{
}

void player_overlay::do_emit_scene()
{
	if(device == nullptr)
	{
		_com_raise_error(D3DERR_INVALIDCALL);
	}
	try
	{
		LOCK(cs);

		if(components.size() != 0)
		{
			SIZE overlay_size(player->get_window_dimensions());
			float dx((-static_cast<float>(overlay_size.cx) / 2.0f));
			float dy((-static_cast<float>(overlay_size.cy) / 2.0f));

			D3DXMATRIX original_translation;
			FAIL_THROW(device->GetTransform(D3DTS_WORLD, &original_translation));
			ON_BLOCK_EXIT(device->SetTransform(D3DTS_WORLD, &original_translation));

			D3DXMATRIX translation;
			D3DXMatrixTranslation(&translation, dx, dy, 0.0f);
			D3DXMATRIX rotation;
			D3DXMatrixRotationX(&rotation, 1.0f * D3DX_PI);
			D3DXMATRIX transformation;
			transformation = translation * rotation;
			clamp_to_zero(&transformation, 0.0001f);
			FAIL_THROW(device->SetTransform(D3DTS_WORLD, &transformation));

			ID3DXSpritePtr sprite;
			::D3DXCreateSprite(device, &sprite);
			
			sprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_OBJECTSPACE);

			for(map_type::const_iterator it(components.begin()), end(components.end()); it != end; ++it)
			{
				it->second->render(sprite, font, overlay_size);
			}
			sprite->End();
		}
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
}
