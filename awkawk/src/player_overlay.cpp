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

player_overlay::player_overlay(awkawk* player_,
                               direct3d_manager* manager_) : direct3d_renderable(manager_),
                                                             player(player_),
                                                             cs("player_overlay"),
                                                             caption_overlay(new overlay_text(L"", overlay_text::centre, overlay_text::top, false)),
                                                             volume_overlay(new overlay_text(L"", overlay_text::right, overlay_text::bottom, true)),
                                                             button_overlay(new overlay_text(L"", overlay_text::left, overlay_text::bottom, true)),
                                                             position_overlay(new overlay_text(L"", overlay_text::left, overlay_text::top, true))
{
	add_component("caption overlay", caption_overlay);
	add_component("volume overlay", volume_overlay);
	add_component("button overlay", button_overlay);
	add_component("position overlay", position_overlay);
}

player_overlay::~player_overlay()
{
}

void player_overlay::update(float vol, float play_time, active_area active, std::wstring caption_text, RECT caption_position) {

	HDC dc(::CreateCompatibleDC(NULL));
	ON_BLOCK_EXIT(::DeleteDC(dc));
	NONCLIENTMETRICSW metrics = { sizeof(NONCLIENTMETRICSW) };
	::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &metrics, 0);
	HFONT font(::CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, metrics.lfCaptionFont.lfFaceName));
	ON_BLOCK_EXIT(::DeleteObject(font));
	::SelectObject(dc, font);
	std::unique_ptr<wchar_t[]> buffer(new wchar_t[/*caption_text.length() + 1*/ MAX_PATH]);
	std::memset(buffer.get(), 0, sizeof(wchar_t) * MAX_PATH);
	std::copy(caption_text.begin(), caption_text.end(), buffer.get());
	::PathCompactPathW(dc, buffer.get(), caption_position.right - caption_position.left);
	std::wstring compact_caption_text(buffer.get());

	caption_overlay->set_text(compact_caption_text);
	caption_overlay->set_bounds(caption_position);

	{
		if(_finite(vol) && vol != -100.0f)
		{
			std::wstringstream wss;
			wss << vol << L" dB";
			volume_overlay->set_text(wss.str());
		}
		else
		{
			volume_overlay->set_text(L"-∞ dB");
		}
	}
	{
		if(play_time >= 0.0f)
		{
			float seconds(fmod(play_time, 60.0f));
			play_time /= 60.0f;
			play_time = floor(play_time);
			float minutes(fmod(play_time, 60.0f));
			play_time /= 60.0f;
			play_time = floor(play_time);
			float hours(play_time);
			std::wstringstream wss;
			wss << std::setw(2) << std::setfill(L'0') << std::setprecision(0) << std::fixed << hours   << L":"
			    << std::setw(2) << std::setfill(L'0') << std::setprecision(0) << std::fixed << minutes << L":"
			    << std::setw(5) << std::setfill(L'0') << std::setprecision(2) << std::fixed << seconds;
			position_overlay->set_text(wss.str());
		}
		else
		{
			position_overlay->set_text(L"(can't seek)");
		}
	}
	switch(active)
	{
	case volume:
		volume_overlay->set_visible(true);
		break;
	case play:
		button_overlay->set_visible(true);
		button_overlay->set_text(L"Play");
		break;
	case pause:
		button_overlay->set_visible(true);
		button_overlay->set_text(L"Pause");
		break;
	case stop:
		button_overlay->set_visible(true);
		button_overlay->set_text(L"Stop");
		break;
	case rwd:
		button_overlay->set_visible(true);
		button_overlay->set_text(L"Rewind");
		break;
	case ffwd:
		button_overlay->set_visible(true);
		button_overlay->set_text(L"Fast Forward");
		break;
	case timer:
		button_overlay->set_visible(true);
		button_overlay->set_text(L"(timer)");
		break;
	case screen_mode:
		button_overlay->set_visible(true);
		if(player->is_fullscreen())
		{
			button_overlay->set_text(L"Windowed");
		}
		else
		{
			button_overlay->set_text(L"Full screen");
		}
		break;
	case unknown1:
		button_overlay->set_visible(true);
		button_overlay->set_text(L"(unknown)");
		break;
	case unknown2:
		button_overlay->set_visible(true);
		button_overlay->set_text(L"(unknown)");
		break;
	case open:
		button_overlay->set_visible(true);
		button_overlay->set_text(L"Open");
		break;
	case position:
		position_overlay->set_visible(true);
		break;
	}
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

			// 12, 40 come from the skin; height of the titlebar and toolbar, respectively.
			RECT window_bounds = { 0, 12, overlay_size.cx, overlay_size.cy - 40 };

			volume_overlay->set_bounds(window_bounds);
			volume_overlay->set_font(overlay_font);
			button_overlay->set_bounds(window_bounds);
			button_overlay->set_font(overlay_font);
			position_overlay->set_bounds(window_bounds);
			position_overlay->set_font(overlay_font);

			caption_overlay->set_font(caption_font);
			caption_overlay->set_visible(true);

			sprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_OBJECTSPACE);
			layout::render(sprite);
			sprite->End();
		}
	}
	catch(_com_error& ce)
	{
		derr << __FUNCSIG__ << " " << std::hex << ce.Error() << std::endl;
	}
}
