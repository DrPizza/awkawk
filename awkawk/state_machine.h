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

struct async_callback_base
{
	ULONG_PTR message;
	void* arg;

	async_callback_base(ULONG_PTR message_, void* arg_) : message(message_),
	                                                      arg(arg_)
	{
	}

	virtual void execute(void* result) const = 0;

	virtual ~async_callback_base()
	{
	}
};

template<typename T>
struct async_callback : async_callback_base
{
	T* obj;
	typedef void (T::*callback_type)(ULONG_PTR msg, void* arg, void* context, void* result);
	callback_type callback;
	void* context;

	async_callback(T* obj_, callback_type callback_, UINT_PTR message_, void* arg_, void* context_) : async_callback_base(message_, arg_),
	                                                                                                  obj(obj),
	                                                                                                  callback(callback_),
	                                                                                                  context(context_)
	{
	}

	virtual void execute(void* result) const
	{
		return (obj->*callback)(message, arg, context, result);
	}
};

#if 0

template<typename T>
struct message_pump
{
	typedef T handler_type;
	typedef message_pump<handler_type> message_pump_type;
	typedef void* (T::*message_callback_type)(void*);

	enum messages
	{
		send_msg,
		send_msg_timeout,
		send_msg_callback,
		post_msg,
		post_msg_wait,
		post_msg_timeout,
		post_msg_callback,
		post_msg_quit
	};

	message_pump(handler_type* handler_) : handler(handler_),
	                                       post_port(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1)),
	                                       send_port(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1)),
	                                       message_thread(utility::CreateThread(NULL, 0, this, &message_pump_type::message_thread_proc, static_cast<void*>(0), "Message thread", 0, &message_thread_id))
	{
	}

	~message_pump()
	{
		::PostQueuedCompletionStatus(post_port, post_msg_quit, 0, NULL);
		::WaitForSingleObject(message_thread, INFINITE);
		::CloseHandle(message_thread);
		::CloseHandle(send_port);
		::CloseHandle(post_port);
	}

	bool in_send_message() const
	{
		return ::GetCurrentThreadId() == message_thread_id;
	}

	void* send_message(ULONG_PTR msg, void* arg)
	{
		if(!in_send_message())
		{
			std::auto_ptr<OVERLAPPED> o(new OVERLAPPED());
			HANDLE event(::CreateEventW(NULL, FALSE, FALSE, NULL));
			o->hEvent = event;
			ON_BLOCK_EXIT(&::CloseHandle, event);
			o->Pointer = arg;
			::PostQueuedCompletionStatus(send_port, send_msg, msg, o.get());
			::PostQueuedCompletionStatus(post_port, send_msg, 0, 0);
			::WaitForSingleObject(event, INFINITE);
			return o->Pointer;
		}
		else
		{
			return process_message(msg, arg);
		}
	}

	struct send_message_timeout_helper
	{
		void* arg;
		void* result;
		HANDLE completed;
		HANDLE complete_received;
		bool timed_out;

		send_message_timeout_helper(void* arg_) : arg(arg_),
		                                          result(NULL),
		                                          completed(::CreateEventW(NULL, FALSE, FALSE, NULL)),
		                                          complete_received(::CreateEventW(NULL, FALSE, FALSE, NULL)),
		                                          timed_out(false)
		{
		}

		~send_message_timeout_helper()
		{
			::CloseHandle(complete_received);
			::CloseHandle(completed);
		}
	};

	void* send_message_timeout(ULONG_PTR msg, void* arg, DWORD timeout)
	{
		if(!in_send_message())
		{
			std::auto_ptr<OVERLAPPED> o(new OVERLAPPED());
			std::auto_ptr<send_message_timeout_helper> smth(new send_message_timeout_helper(arg));

			o->Pointer = smth.get();

			::PostQueuedCompletionStatus(send_port, send_msg_timeout, msg, o);
			::PostQueuedCompletionStatus(post_port, send_msg_timeout, 0, 0);
			
			switch(::WaitForSingleObject(smth->completed, timeout))
			{
			case WAIT_OBJECT_0:
				smth->timed_out = false;
				::SetEvent(smth->complete_received);
				return o->Pointer;
			case WAIT_TIMEOUT:
				smth->timed_out = true;
				::SetEvent(smth->complete_received);
				smth.release();
				o.release();
				return NULL;
			}
		}
		else
		{
			return process_message(msg, arg);
		}
	}

	template<typename T>
	void* send_message_callback(ULONG_PTR msg, void* arg, T* obj, void (T::*callback)(ULONG_PTR msg, void* arg, void* context, void* result), void* context)
	{
		if(!in_send_message())
		{
			std::auto_ptr<OVERLAPPED> o(new OVERLAPPED());
			std::auto_ptr<async_callback<T> > cb(new async_callback<T>(obj, callback, msg, arg, context));
			o->Pointer = cb.release();
			::PostQueuedCompletionStatus(send_port, send_msg_callback, msg, o.release());
			::PostQueuedCompletionStatus(post_port, send_msg_callback, 0, 0);
			return NULL;
		}
		else
		{
			void* result(process_message(msg, arg));
			(obj->*callback)(msg, arg, context, result);
			return result;
		}
	}

	void post_message(ULONG_PTR msg, void* arg)
	{
		std::auto_ptr<OVERLAPPED> o(new OVERLAPPED());
		o->Pointer = arg;
		::PostQueuedCompletionStatus(post_port, post_msg, static_cast<ULONG_PTR>(msg), o.release());
	}

	void post_message_and_wait(ULONG_PTR msg, void* arg)
	{
		if(in_send_message())
		{
			::DebugBreak();
		}

		std::auto_ptr<OVERLAPPED> o(new OVERLAPPED());
		HANDLE event(::CreateEventW(NULL, FALSE, FALSE, NULL));
		o->hEvent = event;
		ON_BLOCK_EXIT(&::CloseHandle, event);
		o->Pointer = arg;
		::PostQueuedCompletionStatus(post_port, post_msg_wait, msg, o.release());
		::WaitForSingleObject(event, INFINITE);
	}

	void post_message_and_wait(ULONG_PTR msg, void* arg, DWORD timeout)
	{
		if(in_send_message())
		{
			::DebugBreak();
		}

		std::auto_ptr<OVERLAPPED> o(new OVERLAPPED());
		std::auto_ptr<send_message_timeout_helper> smth(new send_message_timeout_helper(arg));
		o->Pointer = smth.release();

		::PostQueuedCompletionStatus(post_port, post_msg_wait, msg, o.release());
		switch(::WaitForSingleObject(smth->completed, timeout))
		{
		case WAIT_OBJECT_0:
			smth->timed_out = false;
			::SetEvent(smth->complete_received);
			break;
		case WAIT_TIMEOUT:
			smth->timed_out = true;
			::SetEvent(smth->complete_received);
			break;
		}
	}

	template<typename T>
	void post_message_with_callback(ULONG_PTR msg, void* arg, T* obj, void (T::*callback)(ULONG_PTR msg, void* arg, void* context, void* result), void* context)
	{
		std::auto_ptr<OVERLAPPED> o(new OVERLAPPED());
		std::auto_ptr<async_callback<T> > cb(new async_callback<T>(obj, callback, msg, arg, context));
		o->Pointer = cb.release();
		::PostQueuedCompletionStatus(send_port, send_msg_callback, msg, o.release());
	}

private:
	void* process_message(ULONG_PTR msg, void* arg)
	{
		return (handler->*(handler->get_callback(msg)))(arg);
	}

	handler_type* handler;
	HANDLE post_port;
	HANDLE send_port;
	HANDLE message_thread;
	DWORD message_thread_id;

	DWORD message_thread_proc(void*)
	{
		bool quit_received(false);
		DWORD posted_action(0);
		ULONG_PTR posted_message(0);
		OVERLAPPED* posted_overlapped(NULL);
		while(FALSE != ::GetQueuedCompletionStatus(post_port, &posted_action, &posted_message, &posted_overlapped, quit_received ? 0 : INFINITE))
		{
			DWORD sent_action(0);
			ULONG_PTR sent_message(0);
			OVERLAPPED* sent_overlapped(NULL);
			while(FALSE != ::GetQueuedCompletionStatus(send_port, &sent_action, &sent_message, &sent_overlapped, 0))
			{
				switch(sent_action)
				{
				case send_msg:
					{
						sent_overlapped->Pointer = process_message(sent_message, sent_overlapped->Pointer);
						::SetEvent(sent_overlapped->hEvent);
					}
					break;
				case send_msg_timeout:
					{
						std::auto_ptr<OVERLAPPED> o(sent_overlapped);
						std::auto_ptr<send_message_timeout_helper> smth(static_cast<send_message_timeout_helper*>(sent_overlapped->Pointer));
						smth->result = process_message(sent_message, smth->arg);
						::SignalObjectAndWait(smth->completed, smth->complete_received, INFINITE, FALSE);
						if(!smth->timed_out)
						{
							o.release();
							smth.release();
						}
					}
					break;
				case send_msg_callback:
					{
						std::auto_ptr<OVERLAPPED> o(sent_overlapped);
						std::auto_ptr<async_callback_base> cb(reinterpret_cast<async_callback_base*>(sent_overlapped->Pointer));
						sent_overlapped->Pointer = process_message(sent_message, cb->arg);
						cb->execute(sent_overlapped->Pointer);
					}
					break;
				}
			}

			switch(posted_action)
			{
			case send_msg:
			case send_msg_timeout:
			case send_msg_callback:
				break;
			case post_msg:
				{
					std::auto_ptr<OVERLAPPED> o(posted_overlapped);
					process_message(posted_message, posted_overlapped->Pointer);
				}
				break;
			case post_msg_wait:
				{
					posted_overlapped->Pointer = process_message(posted_message, posted_overlapped->Pointer);
					::SetEvent(posted_overlapped->hEvent);
				}
				break;
			case post_msg_timeout:
				{
					std::auto_ptr<OVERLAPPED> o(posted_overlapped);
					std::auto_ptr<send_message_timeout_helper> smth(static_cast<send_message_timeout_helper*>(posted_overlapped->Pointer));
					smth->result = process_message(posted_message, smth->arg);
					::SignalObjectAndWait(smth->completed, smth->complete_received, INFINITE, FALSE);
				}
				break;
			case post_msg_callback:
				{
					std::auto_ptr<OVERLAPPED> o(posted_overlapped);
					std::auto_ptr<async_callback_base> cb(reinterpret_cast<async_callback_base*>(posted_overlapped->Pointer));
					posted_overlapped->Pointer = process_message(posted_message, cb->arg);
					cb->execute(posted_overlapped->Pointer);
				}
				break;
			case post_msg_quit:
				quit_received = true;
				break;
			}
		}
		return 0;
	}
};

#else

struct message_pump_base : boost::noncopyable
{
	message_pump_base() : window_created_event(::CreateEventW(NULL, FALSE, FALSE, NULL))
	{
	}

	virtual ~message_pump_base()
	{
	}

	enum messages
	{
		send_msg = WM_USER + 1,
		send_msg_callback,
		send_msg_timeout,
		send_msg_notify,
		post_msg,
		post_msg_wait,
		post_msg_timeout,
		post_msg_callback,
		post_msg_quit
	};

	void* execute_directly(ULONG_PTR msg, void* arg)
	{
		return process_message(msg, arg);
	}

	void* send_message(ULONG_PTR msg, void* arg)
	{
		return reinterpret_cast<void*>(::SendMessageW(get_window(), send_msg, static_cast<WPARAM>(msg), reinterpret_cast<LPARAM>(arg)));
	}

	template<typename T>
	BOOL send_message_callback(ULONG_PTR msg, void* arg, T* obj, void (T::*callback)(ULONG_PTR msg, void* arg, void* context, void* result), void* context)
	{
		std::auto_ptr<async_callback<T> > cb(new async_callback<T>(obj, callback, msg, arg, context));
		return ::SendMessageCallbackW(get_window(), send_msg_callback, static_cast<WPARAM>(msg), reinterpret_cast<LPARAM>(arg), &message_pump_base::send_async_helper<T>, reinterpret_cast<ULONG_PTR>(cb.release()));
	}

	void* send_message_timeout(ULONG_PTR msg, void* arg, UINT flags, UINT timeout, DWORD_PTR* result)
	{
		return reinterpret_cast<void*>(::SendMessageTimeoutW(get_window(), send_msg_timeout, static_cast<WPARAM>(msg), reinterpret_cast<LPARAM>(arg), flags, timeout, result));
	}

	BOOL send_notify_message(ULONG_PTR msg, void* arg)
	{
		return ::SendNotifyMessageW(get_window(), send_msg_notify, static_cast<WPARAM>(msg), reinterpret_cast<LPARAM>(arg));
	}

	BOOL post_message(ULONG_PTR msg, void* arg)
	{
		return ::PostMessageW(get_window(), post_msg, static_cast<WPARAM>(msg), reinterpret_cast<LPARAM>(arg));
	}

	template<typename T>
	BOOL post_message_callback(ULONG_PTR msg, void* arg, T* obj, void (T::*callback)(ULONG_PTR msg, void* arg, void* context, void* result), void* context)
	{
		std::auto_ptr<async_callback<T> > cb(new async_callback<T>(obj, callback, msg, arg, context));
		return ::PostMessageW(get_window(), post_msg_callback, 0, reinterpret_cast<LPARAM>(cb.release()));
	}

protected:
	void* process_message(ULONG_PTR msg, void* arg)
	{
		return do_process_message(msg, arg);
	}

	virtual void* do_process_message(ULONG_PTR msg, void* arg) = 0;

	void initialize()
	{
		message_thread = utility::CreateThread(NULL, 0, this, &message_pump_base::message_thread_proc, static_cast<void*>(0), "Message thread", 0, NULL);
		::WaitForSingleObject(window_created_event, INFINITE);
	}

	void uninitialize()
	{
		::SendMessageW(get_window(), post_msg_quit, 0, 0);
		::WaitForSingleObject(message_thread, INFINITE);
		::CloseHandle(message_thread);
		::CloseHandle(window_created_event);
	}

	void register_class()
	{
		::memset(&window_class, 0, sizeof(window_class));
		window_class.cbSize = sizeof(WNDCLASSEXW);
		window_class.hInstance = ::GetModuleHandle(NULL);
		window_class.lpfnWndProc = &message_pump_base::message_proc_helper;
		window_class.cbClsExtra = 0;
		window_class.cbWndExtra = sizeof(message_pump_base*);
		window_class.lpszClassName = L"message_pump_class";

		if(0 == ::RegisterClassExW(&window_class))
		{
			switch(::GetLastError())
			{
			case ERROR_CLASS_ALREADY_EXISTS:
				registered = false;
				break;
			default:
				dout << ::GetLastError() << std::endl;
				throw std::runtime_error("Could not register window class");
			}
		}
		else
		{
			registered = true;
		}
	}

	void unregister_class()
	{
		if(registered)
		{
			::UnregisterClassW(window_class.lpszClassName, ::GetModuleHandle(NULL));
		}
	}

	const wchar_t* get_class_name() const
	{
		return window_class.lpszClassName;
	}

	void create_window(void* param)
	{
		std::auto_ptr<std::pair<void*, void*> > ptrs(new std::pair<void*, void*>(this, param));
		set_window(::CreateWindowExW(0, get_class_name(), L"message-only window", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, ::GetModuleHandle(NULL), ptrs.get()));
		// by the time CreateWindowExW returns, WM_NCCREATE has already been processed, so it's always safe to delete the memory here
	}

	virtual LRESULT CALLBACK message_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;

	HWND get_window() const
	{
		return message_window;
	}

	void set_window(HWND wnd)
	{
		message_window = wnd;
	}

private:
	static LRESULT CALLBACK message_proc_helper(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)
		{
		case WM_NCCREATE: //sadly this is not actually the first message the window receives, just the first one that's useful.  user32, sucking it hardcore, as ever.
			{
				CREATESTRUCTW* cs(reinterpret_cast<CREATESTRUCTW*>(lParam));
				std::pair<void*, void*>* ptrs(reinterpret_cast<std::pair<void*, void*>*>(cs->lpCreateParams));
				::SetWindowLongPtrW(wnd, 0, reinterpret_cast<LONG_PTR>(ptrs->first));
				cs->lpCreateParams = ptrs->second;
			}
		default:
			{
				message_pump_base* mp(reinterpret_cast<message_pump_base*>(::GetWindowLongPtrW(wnd, 0)));
				if(mp)
				{
					return mp->message_proc(wnd, message, wParam, lParam);
				}
				else
				{
					return ::DefWindowProcW(wnd, message, wParam, lParam);
				}
			}
		}
	}

	template<typename T>
	static void CALLBACK send_async_helper(HWND wnd, UINT message, ULONG_PTR context, LRESULT lResult)
	{
		std::auto_ptr<async_callback<T> > cb(reinterpret_cast<async_callback<T>*>(context));
		return cb->execute(reinterpret_cast<void*>(lResult));
	}

	DWORD message_thread_proc(void*)
	{
		register_class();
		create_window(NULL);

		MSG msg = {0};
		::PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
		::SetEvent(window_created_event);

		while(BOOL rv = ::GetMessageW(&msg, NULL, 0, 0))
		{
			if(rv == -1)
			{
				return static_cast<DWORD>(-1);
			}
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}

		unregister_class();

		return static_cast<DWORD>(msg.wParam);
	}

	HANDLE window_created_event;
	HANDLE message_thread;

	WNDCLASSEXW window_class;
	bool registered;
	HWND message_window;
};

template<typename T>
struct message_pump : message_pump_base
{
	typedef T handler_type;
	typedef message_pump<handler_type> message_pump_type;
	typedef void* (T::*message_callback_type)(void*);

	message_pump(handler_type* handler_) : handler(handler_)
	{
		initialize();
	}

	virtual ~message_pump()
	{
		uninitialize();
	}

protected:
	void* do_process_message(ULONG_PTR msg, void* arg)
	{
		return (handler->*(handler->get_callback(msg)))(arg);
	}

	virtual LRESULT CALLBACK message_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)
		{
		case send_msg:
			return reinterpret_cast<LRESULT>(process_message(static_cast<UINT_PTR>(wParam), reinterpret_cast<void*>(lParam)));
		case send_msg_callback:
			{
				std::auto_ptr<async_callback_base> cb(reinterpret_cast<async_callback_base*>(lParam));
				LRESULT result(reinterpret_cast<LRESULT>(process_message(cb->message, cb->arg)));
				cb->execute(reinterpret_cast<void*>(result));
				return result;
			}
		case send_msg_timeout:
			return reinterpret_cast<LRESULT>(process_message(static_cast<UINT_PTR>(wParam), reinterpret_cast<void*>(lParam)));
		case send_msg_notify:
			return reinterpret_cast<LRESULT>(process_message(static_cast<UINT_PTR>(wParam), reinterpret_cast<void*>(lParam)));
		case post_msg:
			return reinterpret_cast<LRESULT>(process_message(static_cast<UINT_PTR>(wParam), reinterpret_cast<void*>(lParam)));
		//case post_msg_wait:
		//case post_msg_timeout:
		case post_msg_callback:
			{
				std::auto_ptr<async_callback_base> cb(reinterpret_cast<async_callback_base*>(lParam));
				LRESULT result(reinterpret_cast<LRESULT>(process_message(cb->message, cb->arg)));
				cb->execute(reinterpret_cast<void*>(result));
				return result;
			}
		case post_msg_quit:
			::PostQuitMessage(0);
			return 0;
		default:
			return ::DefWindowProcW(wnd, message, wParam, lParam);
		}
	}
private:
	handler_type* handler;
};

#endif

template<typename T, typename S>
struct transition
{
	typedef T handler_type;
	typedef S state_type;

	typedef size_t (handler_type::*state_fun)();
	typedef array<state_type> state_array;

	state_fun fun;
	state_array next_states;

	state_type execute(handler_type* hnd) const
	{
		size_t next((hnd->*fun)());
		return next_states.length > 0 && (next != ~0) ? next_states[next]
		                                              : hnd->get_current_state();
	}
};

template<typename T, typename S, typename E>
struct event_handler : boost::noncopyable
{
	typedef T handler_type;
	typedef S state_type;
	typedef E event_type;
	typedef event_handler<handler_type, state_type, event_type> event_handler_type;
	typedef message_pump<event_handler_type> message_pump_type;

	event_handler(handler_type* handler_) : handler(handler_), pump(new message_pump_type(this))
	{
	}

	const typename message_pump_type::message_callback_type get_callback(ULONG_PTR) const
	{
		return &event_handler_type::event_dispatch;
	}

	state_type send_event(event_type evt)
	{
		return static_cast<state_type>(reinterpret_cast<size_t>(pump->send_message(0, reinterpret_cast<void*>(evt))));
	}

	void post_event(event_type evt)
	{
		pump->post_message(0, reinterpret_cast<void*>(evt));
	}

	bool permitted(event_type evt) const
	{
		return handler->get_transition(evt)->next_states.length != 0;
	}

private:
	void* event_dispatch(void* msg)
	{
		event_type evt(static_cast<event_type>(reinterpret_cast<size_t>(msg)));
		if(permitted(evt))
		{
			dout << "current state: " << handler->state_name(handler->get_current_state()) << std::endl;
			handler->set_current_state(handler->get_transition(evt)->execute(handler));
			dout << "new state: " << handler->state_name(handler->get_current_state()) << std::endl;
		}
		else
		{
			dout << "the event " << handler->event_name(evt) << " is not permitted in state " << handler->state_name(handler->get_current_state()) << std::endl;
		}
		return reinterpret_cast<void*>(handler->get_current_state());
	}

	handler_type* handler;
	std::auto_ptr<message_pump_type> pump;
};

#endif
