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

#ifndef WINDOW_HOOK__H
#define WINDOW_HOOK__H

#define NOMINMAX
#define NTDDI_VERSION NTDDI_LONGHORN
#define STRICT
#pragma warning(disable:4995)
#pragma warning(disable:4996)
#include <windows.h>

template<typename T>
HOOKPROC make_hook_proc(const T* object, LRESULT(CALLBACK T::* function)(int, WPARAM, LPARAM))
{
	// TODO: handle the adjustor in the member function pointer?
	union converter
	{
		LRESULT (CALLBACK T::* pointer)(int, WPARAM, LPARAM);
		struct member_function_pointer
		{
			size_t addr;
			size_t adjustor;
		} decomposed;
	};
	SYSTEM_INFO si = {0};
	::GetSystemInfo(&si);
	const converter c = { function };
	void* base_address(reinterpret_cast<void*>(c.decomposed.addr));
	MEMORY_BASIC_INFORMATION mbi = {0};
	::VirtualQuery(base_address, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
	base_address = reinterpret_cast<char*>(mbi.AllocationBase);
	// this isn't really necessary for IA32, but it is for x86-64, where the trampoline must be within +/- 2 GiB
	// of the function, or else it's too far away for a RIP-relative CALL
	void* function_code(NULL);
	do
	{
		base_address = reinterpret_cast<char*>(base_address) - si.dwAllocationGranularity;
		function_code = ::VirtualAlloc(base_address, si.dwAllocationGranularity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	}
	while(function_code == NULL);
#if defined(_M_IX86)
	static const unsigned char code[] =
	{
		0x55,                                                                   // push        ebp  
		0x8B, 0xEC,                                                             // mov         ebp,esp 
		0x51,                                                                   // push        ecx  
		0xC7, 0x45, 0xFC, 0xCC, 0xCC, 0xCC, 0xCC,                               // mov         dword ptr [ebp-4],0CCCCCCCCh 
		0xC7, 0x45, 0xFC, /* 'this' pointer */ 0xEF, 0xBE, 0xAD, 0xDE,          // mov         dword ptr [ebi],0DEADBEEFh 
		0x8B, 0x45, 0x10,                                                       // mov         eax,dword ptr [lParam] 
		0x50,                                                                   // push        eax  
		0x8B, 0x4D, 0x0C,                                                       // mov         ecx,dword ptr [wParam] 
		0x51,                                                                   // push        ecx  
		0x8B, 0x55, 0x08,                                                       // mov         edx,dword ptr [code] 
		0x52,                                                                   // push        edx  
		0x8B, 0x45, 0xFC,                                                       // mov         eax,dword ptr [ebi] 
		0x50,                                                                   // push        eax  
		0xE8, /* IP-relative offset to function */ 0xEF, 0xBE, 0xAD, 0xDE,      // call        T::function 
		0x83, 0xC4, 0x04,                                                       // add         esp,4 
		0x3B, 0xEC,                                                             // cmp         ebp,esp 
		0xE8, /* IP-relative offset to _RTC_CheckEsp */ 0xEF, 0xBE, 0xAD, 0xDE, // call        _RTC_CheckEsp 
		0x8B, 0xE5,                                                             // mov         esp,ebp 
		0x5D,                                                                   // pop         ebp  
		0xC2, 0x0C, 0x00                                                        // ret         0Ch  
	};
	std::memcpy(function_code, code, sizeof(code));
	*reinterpret_cast<unsigned __int32*>(reinterpret_cast<char*>(function_code) + 14) = reinterpret_cast<unsigned __int32>(object);
	ptrdiff_t distance(c.decomposed.addr - reinterpret_cast<ptrdiff_t>(reinterpret_cast<char*>(function_code) + 39));
	*reinterpret_cast<signed __int32*>(reinterpret_cast<char*>(function_code) + 35) = static_cast<signed __int32>(distance);
	distance = reinterpret_cast<ptrdiff_t>(&_RTC_CheckEsp) - reinterpret_cast<ptrdiff_t>(reinterpret_cast<char*>(function_code) + 49);
	*reinterpret_cast<signed __int32*>(reinterpret_cast<char*>(function_code) + 45) = static_cast<signed __int32>(distance);
#elif defined(_M_X64)
	static const unsigned char code[] =
	{
		0x4C, 0x89, 0x44, 0x24, 0x18,                                                    // mov         qword ptr [rsp+18h],r8 
		0x48, 0x89, 0x54, 0x24, 0x10,                                                    // mov         qword ptr [rsp+10h],rdx 
		0x89, 0x4C, 0x24, 0x08,                                                          // mov         dword ptr [rsp+8],ecx 
		0x57,                                                                            // push        rdi  
		0x48, 0x83, 0xEC, 0x30,                                                          // sub         rsp,30h 
		0x48, 0x8B, 0xFC,                                                                // mov         rdi,rsp 
		0x48, 0xB9, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                      // mov         rcx,0Ch 
		0xB8, 0xCC, 0xCC, 0xCC, 0xCC,                                                    // mov         eax,0CCCCCCCCh 
		0xF3, 0xAB,                                                                      // rep stos    dword ptr [rdi] 
		0x8B, 0x4C, 0x24, 0x40,                                                          // mov         ecx,dword ptr [rsp+40h] 
		0x48, 0xB8, /* 'this' pointer */ 0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE, // mov         rax,0DEADBEEFDEADBEEFh 
		0x48, 0x89, 0x44, 0x24, 0x20,                                                    // mov         qword ptr [ebi],rax 
		0x4C, 0x8B, 0x4C, 0x24, 0x50,                                                    // mov         r9,qword ptr [lParam] 
		0x4C, 0x8B, 0x44, 0x24, 0x48,                                                    // mov         r8,qword ptr [wParam] 
		0x8B, 0x54, 0x24, 0x40,                                                          // mov         edx,dword ptr [code] 
		0x48, 0x8B, 0x4C, 0x24, 0x20,                                                    // mov         rcx,qword ptr [ebi] 
		0xE8, /* IP-relative offset to member_hook_proc */ 0xEF, 0xBE, 0xAD, 0xDE,       // call        T::function 
		0x48, 0x83, 0xC4, 0x30,                                                          // add         rsp,30h 
		0x5F,                                                                            // pop         rdi  
		0xC3                                                                             // ret              
	};
	std::memcpy(function_code, code, sizeof(code));
	*reinterpret_cast<unsigned __int64*>(reinterpret_cast<char*>(function_code) + 45) = reinterpret_cast<unsigned __int64>(object);
	ptrdiff_t distance(c.decomposed.addr - reinterpret_cast<ptrdiff_t>(reinterpret_cast<char*>(function_code) + 82));
	*reinterpret_cast<signed __int32*>(reinterpret_cast<char*>(function_code) + 78) = static_cast<signed __int32>(distance & 0xffffffff);
#else
#error Unsupported architecture
#endif
	DWORD old_protection;
	::VirtualProtect(function_code, si.dwAllocationGranularity, PAGE_EXECUTE_READ, &old_protection);
	return reinterpret_cast<HOOKPROC>(function_code);
}

inline void destroy_hook_proc(HOOKPROC function_code)
{
	::VirtualFree(reinterpret_cast<void*>(function_code), 0, MEM_RELEASE);
}

#endif
