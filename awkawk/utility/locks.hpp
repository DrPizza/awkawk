//  Copyright (C) 2002-2007 Peter Bright
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

#ifndef LOCKS_HPP
#define LOCKS_HPP

#define NOMINMAX
#define NTDDI_VERSION NTDDI_VISTA
#define STRICT
#pragma warning(disable:4189) // local initialized but not referenced

#include <windows.h>

#include <list>
#include <map>
#include <set>
#include <boost/shared_ptr.hpp>

#include "debug.hpp"
#include "iterator.hpp"
#include "loki/ScopeGuard.h"
#include "loki/ScopeGuardExt.h"

#if defined DEBUG || defined _DEBUG
#define TRACK_LOCKS
#endif

namespace utility
{

struct critical_section;

struct lock_tracker
{
	struct lock_manipulation
	{
		void* return_address;
		void* address_of_return_address;

		const critical_section* section;
		enum lock_operation
		{
			attempt,
			fail,
			acquire,
			release
		};
		lock_operation operation;
		long depth;

		lock_manipulation(void* return_address_, void* address_of_return_address_, const critical_section* section_, lock_operation operation_, long depth_) : return_address(return_address_),
		                                                                                                                                                       address_of_return_address(address_of_return_address_),
		                                                                                                                                                       section(section_),
		                                                                                                                                                       operation(operation_),
		                                                                                                                                                       depth(depth_)
		{
		}
		lock_manipulation()
		{
		}
		bool operator==(const lock_manipulation& rhs) const
		{
			return return_address == rhs.return_address && address_of_return_address == rhs.address_of_return_address && section == rhs.section && operation == rhs.operation && depth == rhs.depth;
		}
		template<typename T>
		friend std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const lock_tracker::lock_manipulation& lm);
	};

	struct lock_info
	{
		lock_tracker* tracker;
		CRITICAL_SECTION cs;
		std::list<lock_manipulation> current_sequence;
		long outstanding_locks;

		lock_info(lock_tracker* tracker_) : tracker(tracker_), outstanding_locks(0)
		{
			::InitializeCriticalSection(&cs);
		}
		~lock_info()
		{
			::DeleteCriticalSection(&cs);
		}
		void add_entry(void* return_address, void* address_of_return_address, const critical_section* section, lock_manipulation::lock_operation operation)
		{
			current_sequence.push_back(lock_manipulation(return_address, address_of_return_address, section, operation, outstanding_locks));
		}
		void add_attempt(void* return_address, void* address_of_return_address, const critical_section* section)
		{
			::EnterCriticalSection(&cs);
			ON_BLOCK_EXIT(::LeaveCriticalSection, &cs);
			add_entry(return_address, address_of_return_address, section, lock_manipulation::attempt);
		}
		void add_fail(void* return_address, void* address_of_return_address, const critical_section* section)
		{
			::EnterCriticalSection(&cs);
			ON_BLOCK_EXIT(::LeaveCriticalSection, &cs);
			add_entry(return_address, address_of_return_address, section, lock_manipulation::fail);
		}
		void add_acquire(void* return_address, void* address_of_return_address, const critical_section* section)
		{
			::EnterCriticalSection(&cs);
			ON_BLOCK_EXIT(::LeaveCriticalSection, &cs);
			add_entry(return_address, address_of_return_address, section, lock_manipulation::acquire);
			::InterlockedIncrement(&outstanding_locks);
		}
		void add_release(void* return_address, void* address_of_return_address, const critical_section* section)
		{
			::EnterCriticalSection(&cs);
			ON_BLOCK_EXIT(::LeaveCriticalSection, &cs);
			::InterlockedDecrement(&outstanding_locks);
			add_entry(return_address, address_of_return_address, section, lock_manipulation::release);
			if(outstanding_locks == 0)
			{
				tracker->add_sequence(current_sequence);
				current_sequence.clear();
			}
		}
	private:
		lock_info(const lock_info&);
	};

	std::map<DWORD, boost::shared_ptr<lock_info> > info;
	std::list<std::list<lock_manipulation> > lock_sequences;
	CRITICAL_SECTION cs;

	lock_info& get_lock_info()
	{
		::EnterCriticalSection(&cs);
		ON_BLOCK_EXIT(&::LeaveCriticalSection, &cs);
		if(info[::GetCurrentThreadId()].get() == NULL)
		{
			info[::GetCurrentThreadId()].reset(new lock_info(this));
		}
		return *info[::GetCurrentThreadId()];
	}

	void attempt(void* return_address, void* address_of_return_address, const critical_section* section)
	{
		get_lock_info().add_attempt(return_address, address_of_return_address, section);
	}

	void fail(void* return_address, void* address_of_return_address, const critical_section* section)
	{
		get_lock_info().add_fail(return_address, address_of_return_address, section);
	}

	void acquire(void* return_address, void* address_of_return_address, const critical_section* section)
	{
		get_lock_info().add_acquire(return_address, address_of_return_address, section);
	}

	void release(void* return_address, void* address_of_return_address, const critical_section* section)
	{
		get_lock_info().add_release(return_address, address_of_return_address, section);
	}

	void add_sequence(const std::list<lock_manipulation>& seq)
	{
		::EnterCriticalSection(&cs);
		ON_BLOCK_EXIT(&::LeaveCriticalSection, &cs);
		if(lock_sequences.end() == std::find(lock_sequences.begin(), lock_sequences.end(), seq))
		{
			lock_sequences.push_back(seq);
		}
	}

	lock_tracker()
	{
		::InitializeCriticalSection(&cs);
		::SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_CASE_INSENSITIVE);
		::SymInitializeW(::GetCurrentProcess(), NULL, TRUE);
	}

	~lock_tracker()
	{
		::SymCleanup(::GetCurrentProcess());
		::DeleteCriticalSection(&cs);
	}

	struct cs_sequence
	{
		std::list<lock_tracker::lock_manipulation> sequence;
		const std::list<lock_tracker::lock_manipulation>* manipulations;

		cs_sequence(std::list<lock_tracker::lock_manipulation> sequence_, const std::list<lock_tracker::lock_manipulation>* manipulations_) : sequence(sequence_),
		                                                                                                                                      manipulations(manipulations_)
		{
		}
		cs_sequence()
		{
		}
		bool operator==(const cs_sequence& rhs) const
		{
			if(sequence.size() != rhs.sequence.size())
			{
				return false;
			}
			for(std::list<lock_tracker::lock_manipulation>::const_iterator lit(sequence.begin()), rit(rhs.sequence.begin()), end(sequence.end()); lit != end; ++lit, ++rit)
			{
				if(lit->section != rit ->section)
				{
					return false;
				}
			}
			return true;
		}
		bool operator<(const cs_sequence& rhs) const
		{
			if(sequence.size() < rhs.sequence.size())
			{
				return true;
			}
			if(sequence.size() > rhs.sequence.size())
			{
				return false;
			}
			for(std::list<lock_tracker::lock_manipulation>::const_iterator lit(sequence.begin()), rit(rhs.sequence.begin()), end(sequence.end()); lit != end; ++lit, ++rit)
			{
				if(lit->section < rit->section)
				{
					return true;
				}
				if(lit->section > rit->section)
				{
					return false;
				}
			}
			return false;
		}
	};

	std::set<std::pair<cs_sequence, cs_sequence> > analyze_deadlocks();

private:
	lock_tracker(const lock_tracker&);
};

template<typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const lock_tracker::lock_manipulation& lm)
{
	for(long i(0); i < lm.depth; ++i)
	{
		os << T(' ');
	}
	utility::print_caller_info(os, lm.return_address, lm.address_of_return_address);
	os << T(':') << T(' ');
	switch(lm.operation)
	{
	case lock_tracker::lock_manipulation::attempt:
		os << T('a') << T('t') << T('t') << T('e') << T('m') << T('p') << T('t');
		break;
	case lock_tracker::lock_manipulation::acquire:
		os << T('a') << T('c') << T('q') << T('u') << T('i') << T('r') << T('e');
		break;
	case lock_tracker::lock_manipulation::fail:
		os << T('f') << T('a') << T('i') << T('l');
		break;
	case lock_tracker::lock_manipulation::release:
		os << T('r') << T('e') << T('l') << T('e') << T('a') << T('s') << T('e');
		break;
	}
	os << T(' ') << lm.section;
	return os;
}

#ifdef TRACK_LOCKS
extern utility::lock_tracker tracker;
#endif

struct critical_section
{
	critical_section(const std::string& name_) : count(0), name(name_)
	{
		::InitializeCriticalSection(&cs);
	}

	~critical_section()
	{
		::DeleteCriticalSection(&cs);
	}

	void enter(void* return_address = _ReturnAddress(), void* address_of_return_address = _AddressOfReturnAddress())
	{
#ifdef TRACK_LOCKS
		tracker.attempt(return_address, address_of_return_address, this);
#else
		UNREFERENCED_PARAMETER(return_address);
		UNREFERENCED_PARAMETER(address_of_return_address);
#endif
		::EnterCriticalSection(&cs);
		::InterlockedIncrement(&count);
#ifdef TRACK_LOCKS
		tracker.acquire(return_address, address_of_return_address, this);
#endif
	}

	bool attempt_enter(void* return_address = _ReturnAddress(), void* address_of_return_address = _AddressOfReturnAddress())
	{
#ifdef TRACK_LOCKS
		tracker.attempt(return_address, address_of_return_address, this);
#else
		UNREFERENCED_PARAMETER(return_address);
		UNREFERENCED_PARAMETER(address_of_return_address);
#endif
		if(TRUE == ::TryEnterCriticalSection(&cs))
		{
			::InterlockedIncrement(&count);
#ifdef TRACK_LOCKS
			tracker.acquire(return_address, address_of_return_address, this);
#endif
			return true;
		}
#ifdef TRACK_LOCKS
		tracker.fail(return_address, address_of_return_address, this);
#endif
		return false;
	}

	void leave(void* return_address = _ReturnAddress(), void* address_of_return_address = _AddressOfReturnAddress())
	{
		::InterlockedDecrement(&count);
		::LeaveCriticalSection(&cs);
#ifdef TRACK_LOCKS
		tracker.release(return_address, address_of_return_address, this);
#else
		UNREFERENCED_PARAMETER(return_address);
		UNREFERENCED_PARAMETER(address_of_return_address);
#endif
	}

	struct lock
	{
		lock(critical_section& crit_) : crit(&crit_),
		                                return_address(_ReturnAddress()),
		                                address_of_return_address(_AddressOfReturnAddress())
		{
			crit->enter(return_address, address_of_return_address);
		}
		lock(const lock& rhs) : crit(rhs.crit),
		                        return_address(_ReturnAddress()),
		                        address_of_return_address(_AddressOfReturnAddress())
		{
			crit->enter(return_address, address_of_return_address);
		}
		~lock()
		{
			crit->leave(return_address, address_of_return_address);
		}
		void* return_address;
		void* address_of_return_address;
		critical_section* crit;
	};

	struct attempt_lock
	{
		attempt_lock(critical_section& crit_) : crit(&crit_),
		                                        return_address(_ReturnAddress()),
		                                        address_of_return_address(_AddressOfReturnAddress()),
		                                        succeeded(crit->attempt_enter(return_address, address_of_return_address))
		{
		}
		~attempt_lock()
		{
			if(succeeded)
			{
				crit->leave(return_address, address_of_return_address);
			}
		}
		attempt_lock(const attempt_lock& rhs) : crit(rhs.crit),
		                                        return_address(_ReturnAddress()),
		                                        address_of_return_address(_AddressOfReturnAddress()),
		                                        succeeded(crit->attempt_enter(return_address, address_of_return_address))
		{
		}
		void* return_address;
		void* address_of_return_address;
		critical_section* crit;
		bool succeeded;
	};

	volatile LONG count;
	CRITICAL_SECTION cs;
	const std::string name;
private:
	critical_section(const critical_section&);
	critical_section& operator=(const critical_section&);
};

#define LOCK(cs) utility::critical_section::lock CREATE_NAME(lck)((cs))

}

#endif // #ifndef LOCKS_HPP
