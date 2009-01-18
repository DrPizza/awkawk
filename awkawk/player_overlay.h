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

struct overlay_text : component
{
	enum horizontal_alignment
	{
		left = DT_LEFT,
		centre = DT_CENTER,
		right = DT_RIGHT
	};

	enum vertical_alignment
	{
		top = DT_TOP,
		middle = DT_VCENTER,
		bottom = DT_BOTTOM
	};

	overlay_text(const std::wstring& text_, horizontal_alignment ha, vertical_alignment va, bool visible_) : text(text_), halign(ha), valign(va), visible(visible_), cs("overlay_text")
	{
		::QueryPerformanceFrequency(&frequency);
		::QueryPerformanceCounter(&last_update);
	}

	virtual RECT get_rectangle(ID3DXSpritePtr sprite, ID3DXFontPtr font, SIZE size) const
	{
		LOCK(cs);
		RECT r = { 1, 12 + 1, size.cx + 1, size.cy - 40 + 1 };
		font->DrawTextW(sprite, text.c_str(), -1, &r, DT_CALCRECT | DT_WORDBREAK | halign | valign, D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f));
		return r;
	}

	void render(ID3DXSpritePtr sprite, ID3DXFontPtr font, SIZE size)
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
		RECT r = { 1, 12 + 1, size.cx + 1, size.cy - 40 + 1 };
		font->DrawTextW(sprite, text.c_str(), -1, &r, DT_WORDBREAK | halign | valign, D3DXCOLOR(0.0f, 0.0f, 0.0f, opacity * 0.75f));
		::SetRect(&r, 0, 12, size.cx, size.cy - 40);
		font->DrawTextW(sprite, text.c_str(), -1, &r, DT_WORDBREAK | halign | valign, D3DXCOLOR(1.0f, 1.0f, 1.0f, opacity));
	}

	void set_text(const std::wstring& text_)
	{
		LOCK(cs);
		text = text_;
	}

	void set_horizontal_alignment(horizontal_alignment halign_)
	{
		LOCK(cs);
		halign = halign_;
	}

	void set_vertical_alignment(vertical_alignment valign_)
	{
		LOCK(cs);
		valign = valign_;
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
	std::wstring text;
	horizontal_alignment halign;
	vertical_alignment valign;
	bool visible;
	LARGE_INTEGER last_update;
	LARGE_INTEGER frequency;
};

struct player_overlay : direct3d_object, layout, boost::noncopyable
{
	player_overlay(awkawk* player_, window* parent_);
	virtual ~player_overlay();

	void render();

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

	active_area get_active_area() const
	{
		return active;
	}

	void set_active_area(active_area area)
	{
		active = area;
	}

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
		::D3DXCreateFontW(device, static_cast<INT>(metrics.lfCaptionFont.lfHeight * 1.5f), 0, FW_NORMAL, 0, FALSE, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, PROOF_QUALITY, DEFAULT_PITCH, metrics.lfCaptionFont.lfFaceName, &font);
		return S_OK;
	}
	// destroy D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_lost()
	{
		font = NULL;
		return S_OK;
	}
	// destroy D3DPOOL_MANAGED resources
	virtual void do_on_device_destroyed()
	{
		device = NULL;
	}

private:
	IDirect3DDevice9Ptr device;

	utility::critical_section cs;

	ID3DXFontPtr font;

	active_area active;

	awkawk* player;
};

#endif
