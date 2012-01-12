//  Copyright (C) 2007 Peter Bright
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

#ifndef COMPONENTS__H
#define COMPONENTS__H

#include "stdafx.h"

_COM_SMARTPTR_TYPEDEF(ID3DXFont, IID_ID3DXFont);
_COM_SMARTPTR_TYPEDEF(ID3DXSprite, IID_ID3DXSprite);

struct component
{
	virtual void render(ID3DXSpritePtr sprite) = 0;

	void set_bounds(const RECT& r) {
		bounding_rect = r;
	}

	RECT& get_bounds() {
		return bounding_rect;
	}

	const RECT& get_bounds() const {
		return bounding_rect;
	}

	virtual ~component()
	{
	}

private:
	RECT bounding_rect;
};

struct text_component : component {
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

	text_component(const std::wstring& text_, horizontal_alignment ha, vertical_alignment va) : text(text_), halign(ha), valign(va) {
	}

	void set_font(ID3DXFontPtr font_) {
		font = font_;
	}

	ID3DXFontPtr get_font() const {
		return font;
	}

	void set_horizontal_alignment(horizontal_alignment halign_)
	{
		halign = halign_;
	}

	horizontal_alignment get_horizontal_alignment() const {
		return halign;
	}

	void set_vertical_alignment(vertical_alignment valign_)
	{
		valign = valign_;
	}

	vertical_alignment get_vertical_alignment() const {
		return valign;
	}

	void set_text(const std::wstring& text_)
	{
		text = text_;
	}

	const std::wstring& get_text() const {
		return text;
	}

private:
	horizontal_alignment halign;
	vertical_alignment valign;

	ID3DXFontPtr font;
	std::wstring text;
};

struct layout
{
	typedef std::map<std::string, std::shared_ptr<component> > map_type;

	virtual void add_component(const std::string& name, std::shared_ptr<component> comp)
	{
		components[name] = comp;
	}

	virtual ~layout()
	{
	}

	virtual void render(ID3DXSpritePtr sprite) {
		for(map_type::const_iterator it(components.begin()), end(components.end()); it != end; ++it)
		{
			it->second->render(sprite);
		}
	}

protected:
	map_type components;
};

struct component_owner
{
	virtual void add_components(layout* lay) = 0;
	virtual ~component_owner()
	{
	}
};

#endif
