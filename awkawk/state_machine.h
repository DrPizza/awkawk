//  Copyright (C) 2008 Peter Bright
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

#ifndef STATE_MACHINE__H
#define STATE_MACHINE__H

#include "stdafx.h"

template<typename T, typename S>
struct transition
{
	typedef T handler_type;
	typedef S state_type;

	typedef size_t (handler_type::*state_fun)(void);
	typedef array<state_type> state_array;

	state_fun fun;
	state_array next_states;

	state_type execute(handler_type* hnd) const
	{
		return next_states.length > 0 ? next_states[(hnd->*fun)()]
		                              : hnd->get_current_state();
	}
};

template<typename T, typename S, typename M>
struct event_handler
{
	typedef T handler_type;
	typedef S state_type;
	typedef M message_type;
	typedef event_handler<handler_type, state_type, message_type> event_handler_type;

	event_handler(handler_type* handler_) : message_port(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1)),
	                                        message_thread(utility::CreateThread(NULL, 0, this, &event_handler_type::message_thread_proc, static_cast<void*>(0), "Message thread", 0, 0)),
	                                        handler(handler_)
	{
	}

	~event_handler()
	{
		::PostQueuedCompletionStatus(message_port, 0, 1, NULL);
		::WaitForSingleObject(message_thread, INFINITE);
		::CloseHandle(message_thread);
		::CloseHandle(message_port);
	}

	bool permitted(message_type evt) const
	{
		return handler_type::transitions[handler->get_current_state()][evt].next_states.length != 0;
	}

	state_type process_message(message_type evt)
	{
		if(permitted(evt))
		{
			dout << "current state: " << handler->state_name(handler->get_current_state()) << std::endl;
			handler->set_current_state(handler_type::transitions[handler->get_current_state()][evt].execute(handler));
			dout << "new state: " << handler->state_name(handler->get_current_state()) << std::endl;
		}
		else
		{
			dout << "the event " << handler->event_name(evt) << " is not permitted in state " << handler->state_name(handler->get_current_state()) << std::endl;
		}
		return handler->get_current_state();
	}

	state_type process_message(DWORD evt)
	{
		return process_message(static_cast<message_type>(evt));
	}

	void post_message(message_type evt)
	{
		dout << "posting " << handler->event_name(evt) << " (" << std::hex << WM_USER + evt << ")" << std::endl;
		::PostQueuedCompletionStatus(message_port, static_cast<DWORD>(evt), 0, NULL);
		dout << "posted " << handler->event_name(evt) << " (" << std::hex << WM_USER + evt << ")" << std::endl;
	}

	state_type send_message(message_type evt)
	{
		OVERLAPPED o = {0};
		o.hEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
		ON_BLOCK_EXIT(&::CloseHandle, o.hEvent);
		dout << "sending " << handler->event_name(evt) << " (" << std::hex << WM_USER + evt << ")" << std::endl;
		::PostQueuedCompletionStatus(message_port, static_cast<DWORD>(evt), 0, &o);
		::WaitForSingleObject(o.hEvent, INFINITE);
		dout << "sent " << handler->event_name(evt) << " (" << std::hex << WM_USER + evt << ")" << std::endl;
		return static_cast<state_type>(reinterpret_cast<size_t>(o.Pointer));
	}

private:
	HANDLE message_port;
	HANDLE message_thread;
	handler_type* handler;

	DWORD message_thread_proc(void*)
	{
		bool quit_received(false);
		DWORD message(0);
		ULONG_PTR key(0);
		OVERLAPPED* overlapped(NULL);
		while(FALSE != ::GetQueuedCompletionStatus(message_port, &message, &key, &overlapped, quit_received ? 0 : INFINITE))
		{
			switch(key)
			{
			case 0:
				{
					state_type new_state(process_message(message));
					if(overlapped != NULL)
					{
						overlapped->Pointer = reinterpret_cast<void*>(static_cast<size_t>(new_state));
						::SetEvent(overlapped->hEvent);
					}
				}
				break;
			case 1:
				quit_received = true;
				break;
			}
		}
		dout << "exiting" << std::endl;
		return 0;
	}
};

#endif
