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

#ifndef PLAYER_OVERLAY__H
#define PLAYER_OVERLAY__H

#include "stdafx.h"
#include "strip.h"
#include "window.h"
#include "components.h"

#include "player_controls.h"

_COM_SMARTPTR_TYPEDEF(ID3DXFont, IID_ID3DXFont);
_COM_SMARTPTR_TYPEDEF(ID3DXSprite, IID_ID3DXSprite);

struct awkawk;

struct overlay_text : text_component
{
	overlay_text(const std::wstring& text_, horizontal_alignment ha, vertical_alignment va, bool shadowed_) : text_component(text_, ha, va), visible(false), shadowed(shadowed_), cs("overlay_text")
	{
		::QueryPerformanceFrequency(&frequency);
		::QueryPerformanceCounter(&last_update);
	}

	void render(ID3DXSpritePtr sprite)
	{
		LOCK(cs);
		if(!visible)
		{
			return;
		}
		LARGE_INTEGER now = {0};
		::QueryPerformanceCounter(&now);
		const double elapsed_time(static_cast<double>(now.QuadPart - last_update.QuadPart) / static_cast<double>(frequency.QuadPart));
		float opacity(1.0f);
		if(0.0 <= elapsed_time && elapsed_time < 0.25)
		{
			opacity = 1.0f;
		}
		else if(0.25 <= elapsed_time && elapsed_time < 1.25)
		{
			opacity = static_cast<float>((1.25 - (elapsed_time - 0.25)) / 1.0);
		}
		else
		{
			visible = false;
			return;
		}

		if(shadowed) {
			RECT r(get_bounds());
			::OffsetRect(&r, 1, 1);
			get_font()->DrawTextW(sprite, get_text().c_str(), -1, &r, DT_WORDBREAK | get_horizontal_alignment() | get_vertical_alignment(), D3DXCOLOR(0.0f, 0.0f, 0.0f, opacity * 0.75f));
			::OffsetRect(&r, -1, -1);
			get_font()->DrawTextW(sprite, get_text().c_str(), -1, &r, DT_WORDBREAK | get_horizontal_alignment() | get_vertical_alignment(), D3DXCOLOR(1.0f, 1.0f, 1.0f, opacity));
		} else {
			RECT r(get_bounds());
			get_font()->DrawTextW(sprite, get_text().c_str(), -1, &r, DT_WORDBREAK | get_horizontal_alignment() | get_vertical_alignment(), D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f));
		}
	}

	void set_visible(bool visible_)
	{
		LOCK(cs);
		visible = visible_;
		if(visible)
		{
			::QueryPerformanceCounter(&last_update);
		}
	}

private:
	mutable utility::critical_section cs;
	bool visible;
	bool shadowed;
	LARGE_INTEGER last_update;
	LARGE_INTEGER frequency;
};

struct player_overlay : direct3d_renderable, layout, boost::noncopyable
{
	player_overlay(awkawk* player_, direct3d_manager* manager_);
	virtual ~player_overlay();

	enum active_area
	{
		none,
		position,
		volume,
		play,
		pause,
		stop,
		rwd,
		ffwd,
		timer,
		screen_mode,
		unknown1,
		unknown2,
		open
	};

	void update(float vol, float play_time, active_area active, std::wstring caption_text, RECT caption_position);

protected:
	// create D3DPOOL_MANAGED resources
	virtual HRESULT do_on_device_created(IDirect3DDevice9Ptr new_device)
	{
		device = new_device;
		return S_OK;
	}
	// create D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_reset()
	{
		NONCLIENTMETRICSW metrics = { sizeof(NONCLIENTMETRICSW) };
		::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &metrics, 0);
		::D3DXCreateFontW(device, static_cast<INT>(metrics.lfCaptionFont.lfHeight * 1.5f), 0, FW_NORMAL, 0, FALSE, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, PROOF_QUALITY, DEFAULT_PITCH, metrics.lfCaptionFont.lfFaceName, &overlay_font);
		::D3DXCreateFontW(device, -12, 0, FW_NORMAL, 0, FALSE, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, PROOF_QUALITY, DEFAULT_PITCH, metrics.lfCaptionFont.lfFaceName, &caption_font);

		return S_OK;
	}
	// destroy D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_lost()
	{
		overlay_font = nullptr;
		caption_font = nullptr;
		return S_OK;
	}
	// destroy D3DPOOL_MANAGED resources
	virtual void do_on_device_destroyed()
	{
		device = nullptr;
	}

	virtual void do_emit_scene();

private:
	IDirect3DDevice9Ptr device;

	utility::critical_section cs;

	std::shared_ptr<overlay_text> caption_overlay;
	std::shared_ptr<overlay_text> volume_overlay;
	std::shared_ptr<overlay_text> button_overlay;
	std::shared_ptr<overlay_text> position_overlay;

	ID3DXFontPtr overlay_font;
	ID3DXFontPtr caption_font;

	awkawk* player;
};

#endif
