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

#ifndef LOAD_URL__H
#define LOAD_URL__H

#include "stdafx.h"
#include "resource.h"
#include "window.h"
#include "util.h"

struct open_url_dialogue : dialogue
{
	open_url_dialogue() : dialogue(), window_class_name(L"LoadURLDialogue")
	{
		register_class(CS_DBLCLKS, NULL, ::LoadCursor(NULL, IDC_ARROW), NULL, nullptr, window_class_name.c_str());
	}

	enum result
	{
		cancelled,
		url,
		browse_local
	};

	LRESULT CALLBACK message_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam, bool& handled);

	std::wstring get_location() const
	{
		return location;
	}

private:
	std::wstring window_class_name;
	std::wstring location;
};

#endif
