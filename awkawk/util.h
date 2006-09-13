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

#ifndef UTIL__H
#define UTIL__H

#include "stdafx.h"

#define FAIL_THROW(x) do { HRESULT CREATE_NAME(hr); if(FAILED(CREATE_NAME(hr) = ( x ))) { _com_raise_error(CREATE_NAME(hr)); } } while(0)
#define FAIL_RET(x) do { if(FAILED(hr = ( x ))) { return hr; } } while(0)
#define CREATE_NAME(name) CREATE_NAME_IND(name, __LINE__)
#define CREATE_NAME_IND(first, second) PASTE_2(first, second)
#define PASTE_2(a, b) a ## b

extern std::ostream& dout;
extern std::wostream& wdout;
extern std::ostream& derr;
extern std::wostream& wderr;

template<typename T>
inline std::basic_ostream<T>& operator<<(std::basic_ostream<T>& lhs, const GUID& rhs)
{
	long nFlags(lhs.flags());
	std::streamsize nWidth(lhs.width());
	T cFill(lhs.fill());

	lhs << std::hex << std::setfill(T('0')) ;
	lhs << std::setw(8) << rhs.Data1 << T('-')
	    << std::setw(4) << rhs.Data2 << T('-') << rhs.Data3 << T('-');
	lhs << std::setw(2) << static_cast<unsigned long>(rhs.Data4[0]) << std::setw(2) << static_cast<unsigned long>(rhs.Data4[1]) << T('-');

	for(int i(2); i < 8; ++i)
	{
		lhs << std::setw(2) << static_cast<unsigned long>(rhs.Data4[i]);
	}

	lhs.flags(nFlags);
	lhs.width(nWidth);
	lhs.fill(cFill);
	return lhs;
}

template<typename T>
inline std::basic_ostream<T>& operator<<(std::basic_ostream<T>& lhs, const RECT& rhs)
{
	return lhs << T('(') << rhs.left << T(',') << T(' ') << rhs.top << T(')') << T(' ') << T('-') << T('>') << T(' ')
	           << T('(') << rhs.right << T(',') << T(' ') << rhs.bottom << T(')');
}

template<typename T>
inline std::basic_ostream<T>& operator<<(std::basic_ostream<T>& lhs, const POINT& rhs)
{
	return lhs << T('(') << rhs.x << T(',') << T(' ') << rhs.y << T(')');
}

template<typename T>
inline std::basic_ostream<T>& operator<<(std::basic_ostream<T>& lhs, const SIZE& rhs)
{
	return lhs << T('{') << rhs.cx << T(',') << T(' ') << rhs.cy << T('}');
}

inline RECT normalize(const RECT& r)
{
	RECT rv = { std::min(r.left, r.right), std::min(r.top, r.bottom), std::max(r.left, r.right), std::max(r.top, r.bottom) };
	return rv;
}

struct device_loss_handler
{
	virtual void begin_device_loss() = 0;
	virtual void end_device_loss(IDirect3DDevice9Ptr) = 0;
};

inline IDirect3DTexture9Ptr load_texture_from_resource(IDirect3DDevice9Ptr device, int resource_id, D3DXIMAGE_INFO* info)
{
	HRSRC res(::FindResource(NULL, MAKEINTRESOURCE(resource_id), L"PNG"));
	DWORD size(::SizeofResource(::GetModuleHandle(NULL), res));
	HGLOBAL glob(::LoadResource(::GetModuleHandle(NULL), res));
	void* data(::LockResource(glob));
	IDirect3DTexture9Ptr texture;
	FAIL_THROW(D3DXCreateTextureFromFileInMemoryEx(device, data, size, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_FROM_FILE, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, info, NULL, &texture));
	return texture;
}

struct critical_section
{
	critical_section() : count(0)
	{
		::InitializeCriticalSection(&cs);
	}

	~critical_section()
	{
		::DeleteCriticalSection(&cs);
	}
	void enter()
	{
		::EnterCriticalSection(&cs);
		::InterlockedIncrement(&count);
	}

	bool attempt_enter()
	{
		if(TRUE == ::TryEnterCriticalSection(&cs))
		{
			::InterlockedIncrement(&count);
			return true;
		}
		return false;
	}
	void leave()
	{
		::InterlockedDecrement(&count);
		::LeaveCriticalSection(&cs);
	}

	struct lock
	{
		lock(critical_section& crit_) : crit(&crit_)
		{
			crit->enter();
		}
		lock(const lock& rhs) : crit(rhs.crit)
		{
			crit->enter();
		}
		~lock()
		{
			crit->leave();
		}
		critical_section* crit;
	};

	struct debug_lock
	{
		__declspec(noinline) debug_lock(critical_section& crit_, const char* location_) : crit(&crit_), location(location_)
		{
			crit->enter();
			//std::cerr << "[" << ::GetCurrentThreadId() << "] " << crit->count << " at " << location << std::endl;
		}
		__declspec(noinline) ~debug_lock()
		{
			crit->leave();
			//std::cerr << "[" << ::GetCurrentThreadId() << "] " << crit->count << " at ~" << location << std::endl;
		}
		__declspec(noinline) debug_lock(const debug_lock& rhs) : crit(rhs.crit), location(rhs.location)
		{
			crit->enter();
			//std::cerr << "[" << ::GetCurrentThreadId() << "] " << crit->count << " at " << location << std::endl;
		}
		__declspec(noinline) debug_lock(const debug_lock& rhs, const char* location_) : crit(rhs.crit), location(location_)
		{
			crit->enter();
			//std::cerr << "[" << ::GetCurrentThreadId() << "] " << crit->count << " at " << location << std::endl;
		}
		critical_section* crit;
		const char* location;
	};

	struct attempt_lock
	{
		attempt_lock(critical_section& crit_) : crit(&crit_), succeeded(crit->attempt_enter())
		{
		}
		~attempt_lock()
		{
			if(succeeded)
			{
				crit->leave();
			}
		}
		attempt_lock(const attempt_lock& rhs) : crit(rhs.crit), succeeded(crit->attempt_enter())
		{
		}
		critical_section* crit;
		bool succeeded;
	};
	
	volatile LONG count;
	CRITICAL_SECTION cs;
private:
	critical_section(const critical_section&);
};

inline bool point_in_circle(POINT pt, POINT centre, float radius)
{
	LONG deltaX(pt.x - centre.x);
	LONG deltaY(pt.y - centre.y);
	return (deltaX * deltaX) + (deltaY * deltaY) < (radius * radius);
}

template<typename T>
T clamp(T value, T min_, T max_)
{
	using std::min;
	using std::max;
	return max(min(value, max_), min_);
}

#endif
