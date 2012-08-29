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

#include "open_url.h"

LRESULT CALLBACK open_url_dialogue::message_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	switch(message)
	{
	case WM_COMMAND:
		switch(GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDCANCEL:
			::EndDialog(wnd, cancelled);
			break;
		case IDOK:
			//::SendDlgItemMessage(wnd, IDC_URL, WM_GETTEXT, 
			{
				size_t message_length(::SendDlgItemMessage(wnd, IDC_URL, WM_GETTEXTLENGTH, 0, 0) + 1);
				std::unique_ptr<wchar_t[]> buffer(new wchar_t[message_length]);
				::SendDlgItemMessage(wnd, IDC_URL, WM_GETTEXT, message_length, reinterpret_cast<LPARAM>(buffer.get()));
				location = buffer.get();
			}
			::EndDialog(wnd, url);
			break;
		case IDC_BROWSE:
			::EndDialog(wnd, browse_local);
			break;
		}
		handled = true;
		return 0;
	default:
		return ::DefDlgProc(wnd, message, wParam, lParam);
	}
}
