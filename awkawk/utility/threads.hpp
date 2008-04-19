//  Copyright (C) 2003 Peter Bright
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

// Description: Win32 CreateThread wrappers, including support for VC++'s "named threads"

#pragma once

#ifndef THREADS__H
#define THREADS__H

#define NOMINMAX
#define NTDDI_VERSION NTDDI_LONGHORN
#define STRICT
#pragma warning(disable:4995)
#pragma warning(disable:4996)
#include <windows.h>

#include <memory>

namespace utility
{

struct ThreadName
{
	DWORD type;
	const char* name;
	DWORD tid;
	DWORD flags;
	enum { MagicNumber = 0x406D1388 };
};

inline void SetThreadName(DWORD tid, const char* name)
{
	ThreadName info = {0x1000, name, tid, 0};
	__try
	{
		::RaiseException(ThreadName::MagicNumber, 0, sizeof(info) / sizeof(const ULONG_PTR*), reinterpret_cast<const ULONG_PTR*>(&info));
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}

template <typename T, typename P, typename F>
struct ThreadInfoMemberFn
{
	typedef T class_type;
	typedef P parameter_type;
	typedef F function_type;
	
	function_type function;
	T* obj;
	const char* name;
	P data;
	ThreadInfoMemberFn(class_type* f, function_type fp, parameter_type d, const char* n) : obj(f), function(fp), data(d), name(n) {}
private:
	ThreadInfoMemberFn<T, P, F>& operator=(const ThreadInfoMemberFn<T, P, F>&);
};

template <typename T, typename P, typename F>
DWORD WINAPI ThreadDispatchMemberFn(void* data)
{
	try
	{
		std::auto_ptr<utility::ThreadInfoMemberFn<T, P, F> > ti(static_cast<utility::ThreadInfoMemberFn<T, P, F>*>(data));
		if(ti->name != 0)
		{
			utility::SetThreadName(static_cast<DWORD>(-1), ti->name);
		}
		return ((ti->obj)->*(ti->function))(ti->data);
	}
	catch(...)
	{
		return 0;
	}
}

template <typename T, typename P, typename P2>
HANDLE CreateThread(SECURITY_ATTRIBUTES* sa, SIZE_T stackSize, T* obj, DWORD(T::* func)(P), P2 data, DWORD flags, DWORD* tid)
{
	typedef DWORD(T::* F)(P);
	std::auto_ptr<utility::ThreadInfoMemberFn<T, P, F> > ti(new utility::ThreadInfoMemberFn<T, P, F>(obj, func, data, NULL));
	HANDLE rv(::CreateThread(sa, stackSize, utility::ThreadDispatchMemberFn<T, P, F>, static_cast<void*>(ti.get()), flags, tid));
	if(NULL != rv) { ti.release(); }
	return rv;
}

template <typename T, typename P, typename P2>
HANDLE CreateThread(SECURITY_ATTRIBUTES* sa, SIZE_T stackSize, T* obj, DWORD(T::* func)(P), P2 data, const char* name, DWORD flags, DWORD* tid)
{
	typedef DWORD(T::* F)(P);
	std::auto_ptr<utility::ThreadInfoMemberFn<T, P, F> > ti(new utility::ThreadInfoMemberFn<T, P, F>(obj, func, data, name));
	HANDLE rv(::CreateThread(sa, stackSize, utility::ThreadDispatchMemberFn<T, P, F>, static_cast<void*>(ti.get()), flags, tid));
	if(NULL != rv) { ti.release(); }
	return rv;
}

template <typename P, typename F>
struct ThreadInfoNonMemberFn
{
	typedef P parameter_type;
	typedef F function_type;
	
	function_type function;
	P data;
	const char* name;
	ThreadInfoNonMemberFn(function_type fp, parameter_type d, const char* n) : function(fp), data(d), name(n) {}
private:
	ThreadInfoNonMemberFn<P, F>& operator=(const ThreadInfoNonMemberFn<P, F>&);
};

template <typename P, typename F>
DWORD WINAPI ThreadDispatchNonMemberFn(void* data)
{
	try
	{
		std::auto_ptr<utility::ThreadInfoNonMemberFn<P, F> > ti(static_cast<utility::ThreadInfoNonMemberFn<P, F>*>(data));
		if(ti->name != NULL)
		{
			utility::SetThreadName(-1, ti->name);
		}
		return (ti->function)(ti->data);
	}
	catch(...)
	{
		return 0;
	}
}

template <typename P, typename P2>
HANDLE CreateThread(SECURITY_ATTRIBUTES* sa, SIZE_T stackSize, DWORD(*func)(P), P2 data, DWORD flags, DWORD* tid)
{
	typedef DWORD(*F)(P);
	std::auto_ptr<utility::ThreadInfoNonMemberFn<P, F> > ti(new utility::ThreadInfoNonMemberFn<P, F>(func, data, NULL));
	HANDLE rv(::CreateThread(sa, stackSize, utility::ThreadDispatchNonMemberFn<P, F>, static_cast<void*>(ti.get()), flags, tid));
	if(NULL != rv) { ti.release(); }
	return rv;
}

template <typename P, typename P2>
HANDLE CreateThread(SECURITY_ATTRIBUTES* sa, SIZE_T stackSize, DWORD(*func)(P), P2 data, const char* name, DWORD flags, DWORD* tid)
{
	typedef DWORD(*F)(P);
	std::auto_ptr<utility::ThreadInfoNonMemberFn<P, F> > ti(new utility::ThreadInfoNonMemberFn<P, F>(func, data, name));
	HANDLE rv(::CreateThread(sa, stackSize, utility::ThreadDispatchNonMemberFn<P, F>, static_cast<void*>(ti.get()), flags, tid));
	if(NULL != rv) { ti.release(); }
	return rv;
}

} // namespace utility

#endif // !THREADS__H
