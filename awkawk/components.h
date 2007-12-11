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
	virtual void render(ID3DXSpritePtr sprite, ID3DXFontPtr font, SIZE size) = 0;
	virtual RECT get_rectangle(ID3DXSpritePtr sprite, ID3DXFontPtr font, SIZE size) const = 0;
	virtual ~component()
	{
	}
};

struct layout
{
	typedef std::map<std::string, boost::shared_ptr<component> > map_type;

	virtual void add_component(const std::string& name, boost::shared_ptr<component> comp)
	{
		components[name] = comp;
	}

	virtual void render() = 0;
	virtual ~layout()
	{
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
