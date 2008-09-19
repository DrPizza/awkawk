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

//#ifdef DEBUG
//utility::basic_locking_ostream<char> dout(std::cout);
//utility::basic_locking_ostream<wchar_t> wdout(std::wcout);
//utility::basic_locking_ostream<char> derr(std::cerr);
//utility::basic_locking_ostream<wchar_t> wderr(std::wcerr);
//#else
//utility::locking_ostream& dout(utility::locked_dout);
//utility::wlocking_ostream& wdout(utility::locked_wdout);
//utility::locking_ostream& derr(utility::locked_dout);
//utility::wlocking_ostream& wderr(utility::locked_wdout);
//#endif
utility::locking_ostream& dout = DebugStreams::Instance().locked_dout;
utility::wlocking_ostream& wdout = DebugStreams::Instance().locked_wdout;
utility::locking_ostream& derr = DebugStreams::Instance().locked_dout;
utility::wlocking_ostream& wderr = DebugStreams::Instance().locked_wdout;
