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
	                                       message_thread(utility::CreateThread(nullptr, 0, this, &message_pump_type::message_thread_proc, nullptr, "Message thread", 0, &message_thread_id))
	{
	}

	~message_pump()
	{
		::PostQueuedCompletionStatus(post_port, post_msg_quit, 0, nullptr);
		::WaitForSingleObject(message_thread, INFINITE);
		::CloseHandle(message_thread);
		::CloseHandle(send_port);
		::CloseHandle(post_port);
	}

	bool in_send_message() const
	{
		return ::GetCurrentThreadId() == message_thread_id;
	}

	void* execute_directly(ULONG_PTR msg, void* arg)
	{
		return process_message(msg, arg);
	}

	void* send_message(ULONG_PTR msg, void* arg)
	{
		if(!in_send_message())
		{
			std::unique_ptr<OVERLAPPED> o(new OVERLAPPED());
			HANDLE event(::CreateEventW(nullptr, FALSE, FALSE, nullptr));
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
		                                          result(nullptr),
		                                          completed(::CreateEventW(nullptr, FALSE, FALSE, nullptr)),
		                                          complete_received(::CreateEventW(nullptr, FALSE, FALSE, nullptr)),
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
			std::unique_ptr<OVERLAPPED> o(new OVERLAPPED());
			std::unique_ptr<send_message_timeout_helper> smth(new send_message_timeout_helper(arg));

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
				return nullptr;
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
			std::unique_ptr<OVERLAPPED> o(new OVERLAPPED());
			std::unique_ptr<async_callback<T> > cb(new async_callback<T>(obj, callback, msg, arg, context));
			o->Pointer = cb.release();
			::PostQueuedCompletionStatus(send_port, send_msg_callback, msg, o.release());
			::PostQueuedCompletionStatus(post_port, send_msg_callback, 0, 0);
			return nullptr;
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
		std::unique_ptr<OVERLAPPED> o(new OVERLAPPED());
		o->Pointer = arg;
		::PostQueuedCompletionStatus(post_port, post_msg, static_cast<ULONG_PTR>(msg), o.release());
	}

	void post_message_and_wait(ULONG_PTR msg, void* arg)
	{
		if(in_send_message())
		{
			::DebugBreak();
		}

		std::unique_ptr<OVERLAPPED> o(new OVERLAPPED());
		HANDLE event(::CreateEventW(nullptr, FALSE, FALSE, nullptr));
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

		std::unique_ptr<OVERLAPPED> o(new OVERLAPPED());
		std::unique_ptr<send_message_timeout_helper> smth(new send_message_timeout_helper(arg));
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
	void post_message_callback(ULONG_PTR msg, void* arg, T* obj, void (T::*callback)(ULONG_PTR msg, void* arg, void* context, void* result), void* context)
	{
		std::unique_ptr<OVERLAPPED> o(new OVERLAPPED());
		std::unique_ptr<async_callback<T> > cb(new async_callback<T>(obj, callback, msg, arg, context));
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
		OVERLAPPED* posted_overlapped(nullptr);
		while(FALSE != ::GetQueuedCompletionStatus(post_port, &posted_action, &posted_message, &posted_overlapped, quit_received ? 0 : INFINITE))
		{
			DWORD sent_action(0);
			ULONG_PTR sent_message(0);
			OVERLAPPED* sent_overlapped(nullptr);
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
						std::unique_ptr<OVERLAPPED> o(sent_overlapped);
						std::unique_ptr<send_message_timeout_helper> smth(static_cast<send_message_timeout_helper*>(sent_overlapped->Pointer));
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
						std::unique_ptr<OVERLAPPED> o(sent_overlapped);
						std::unique_ptr<async_callback_base> cb(reinterpret_cast<async_callback_base*>(sent_overlapped->Pointer));
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
					std::unique_ptr<OVERLAPPED> o(posted_overlapped);
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
					std::unique_ptr<OVERLAPPED> o(posted_overlapped);
					std::unique_ptr<send_message_timeout_helper> smth(static_cast<send_message_timeout_helper*>(posted_overlapped->Pointer));
					smth->result = process_message(posted_message, smth->arg);
					::SignalObjectAndWait(smth->completed, smth->complete_received, INFINITE, FALSE);
				}
				break;
			case post_msg_callback:
				{
					std::unique_ptr<OVERLAPPED> o(posted_overlapped);
					std::unique_ptr<async_callback_base> cb(reinterpret_cast<async_callback_base*>(posted_overlapped->Pointer));
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

#elif 0

struct message_pump_core : boost::noncopyable
{
	virtual void* sync(ULONG_PTR msg, void* arg) = 0;
	virtual void async(ULONG_PTR msg, void* arg) = 0;

	template<typename T>
	void sync_with_callback(ULONG_PTR msg, void* arg, T* obj, void (T::*callback)(ULONG_PTR msg, void* arg, void* context, void* result), void* context)
	{
		std::unique_ptr<async_callback<T> > cb(new async_callback<T>(obj, callback, msg, arg, context));
		return do_sync_with_callback(msg, arg, cb.release());
	}

	template<typename T>
	void async_with_callback(ULONG_PTR msg, void* arg, T* obj, void (T::*callback)(ULONG_PTR msg, void* arg, void* context, void* result), void* context)
	{
		std::unique_ptr<async_callback<T> > cb(new async_callback<T>(obj, callback, msg, arg, context));
		return do_async_with_callback(msg, arg, cb.release());
	}

	void* execute_directly(ULONG_PTR msg, void* arg)
	{
		return process_message(msg, arg);
	}

protected:
	virtual void do_sync_with_callback(ULONG_PTR msg, void* arg, async_callback_base* cb) = 0;
	virtual void do_async_with_callback(ULONG_PTR msg, void* arg, async_callback_base* cb) = 0;

	void* process_message(ULONG_PTR msg, void* arg)
	{
		return do_process_message(msg, arg);
	}

	virtual void* do_process_message(ULONG_PTR msg, void* arg) = 0;
};

struct message_pump_core_win32 : message_pump_core
{
	message_pump_core_win32() : window_created_event(::CreateEventW(nullptr, FALSE, FALSE, nullptr))
	{
		initialize();
	}

	virtual ~message_pump_core_win32()
	{
		uninitialize();
	}

	enum messages
	{
		send_msg = WM_USER + 1,
		send_msg_callback,
		post_msg,
		post_msg_callback,
		post_msg_quit
	};

	virtual void* sync(ULONG_PTR msg, void* arg)
	{
		return reinterpret_cast<void*>(::SendMessageW(get_window(), send_msg, static_cast<WPARAM>(msg), reinterpret_cast<LPARAM>(arg)));
	}

	virtual void async(ULONG_PTR msg, void* arg)
	{
		::PostMessageW(get_window(), post_msg, static_cast<WPARAM>(msg), reinterpret_cast<LPARAM>(arg));
	}

protected:
	virtual void do_sync_with_callback(ULONG_PTR msg, void* arg, async_callback_base* cb)
	{
		::SendMessageW(get_window(), send_msg_callback, 0, reinterpret_cast<LPARAM>(cb));
	}

	virtual void do_async_with_callback(ULONG_PTR msg, void* arg, async_callback_base* cb)
	{
		::PostMessageW(get_window(), post_msg_callback, 0, reinterpret_cast<LPARAM>(cb));
	}

	void initialize()
	{
		message_thread = utility::CreateThread(nullptr, 0, this, &message_pump_core_win32::message_thread_proc, nullptr, "Message thread", 0, NULL);
		::WaitForSingleObject(window_created_event, INFINITE);
		::CloseHandle(window_created_event);
	}

	void uninitialize()
	{
		::SendMessageW(get_window(), post_msg_quit, 0, 0);
		::WaitForSingleObject(message_thread, INFINITE);
		::CloseHandle(message_thread);
	}

	void register_class()
	{
		::memset(&window_class, 0, sizeof(window_class));
		window_class.cbSize = sizeof(WNDCLASSEXW);
		window_class.hInstance = ::GetModuleHandle(NULL);
		window_class.lpfnWndProc = &message_pump_core_win32::message_proc_helper;
		window_class.cbClsExtra = 0;
		window_class.cbWndExtra = sizeof(message_pump_core_win32*);
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
		std::unique_ptr<std::pair<void*, void*> > ptrs(new std::pair<void*, void*>(this, param));
		set_window(::CreateWindowExW(0, get_class_name(), L"message-only window", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, ::GetModuleHandle(NULL), ptrs.get()));
		// by the time CreateWindowExW returns, WM_NCCREATE has already been processed, so it's always safe to delete the memory here
	}

	virtual LRESULT CALLBACK message_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)
		{
		case send_msg:
			return reinterpret_cast<LRESULT>(process_message(static_cast<UINT_PTR>(wParam), reinterpret_cast<void*>(lParam)));
		case send_msg_callback:
			{
				std::unique_ptr<async_callback_base> cb(reinterpret_cast<async_callback_base*>(lParam));
				LRESULT result(reinterpret_cast<LRESULT>(process_message(cb->message, cb->arg)));
				cb->execute(reinterpret_cast<void*>(result));
				return result;
			}
		case post_msg:
			return reinterpret_cast<LRESULT>(process_message(static_cast<UINT_PTR>(wParam), reinterpret_cast<void*>(lParam)));
		case post_msg_callback:
			{
				std::unique_ptr<async_callback_base> cb(reinterpret_cast<async_callback_base*>(lParam));
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
	HWND get_window() const
	{
		return message_window;
	}

	void set_window(HWND wnd)
	{
		message_window = wnd;
	}

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
				message_pump_core_win32* mp(reinterpret_cast<message_pump_core_win32*>(::GetWindowLongPtrW(wnd, 0)));
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

	DWORD message_thread_proc(void*)
	{
		register_class();
		create_window(nullptr);

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

struct message_pump_core_gcd : message_pump_core
{
	message_pump_core_gcd()
	{
	}

	virtual ~message_pump_core_gcd()
	{
	}

	virtual void* sync(ULONG_PTR msg, void* arg)
	{
		void* result = nullptr;
		thread_queue.sync([&] {
			result = process_message(msg, arg);
		});
		return result;
	}

	virtual void async(ULONG_PTR msg, void* arg)
	{
		thread_queue.async([&] {
			process_message(msg, arg);
		});
	}

protected:
	virtual void do_sync_with_callback(ULONG_PTR msg, void* arg, async_callback_base* cb)
	{
		thread_queue.sync([&] {
			void* result(process_message(msg, arg));
			std::unique_ptr<async_callback_base> cb(reinterpret_cast<async_callback_base*>(cb));
			cb->execute(result);
		});
	}

	virtual void do_async_with_callback(ULONG_PTR msg, void* arg, async_callback_base* cb)
	{
		thread_queue.async([&] {
			void* result(process_message(msg, arg));
			std::unique_ptr<async_callback_base> cb(reinterpret_cast<async_callback_base*>(cb));
			cb->execute(result);
		});
	}

private:
	queue_with_own_thread thread_queue;
};

template<typename Base, typename Handler>
struct message_pump : Base
{
	typedef Base base_type;
	typedef Handler handler_type;
	typedef message_pump<base_type, handler_type> message_pump_type;
	typedef void* (handler_type::*message_callback_type)(void*);

	message_pump(handler_type* handler_) : handler(handler_)
	{
	}

	virtual ~message_pump()
	{
	}

protected:
	void* do_process_message(ULONG_PTR msg, void* arg)
	{
		return (handler->*(handler->get_callback(msg)))(arg);
	}

private:
	handler_type* handler;
};

#endif

struct queue_with_own_thread : gcd::queue
{
	queue_with_own_thread() : gcd::queue(nullptr),
	                          window_created_event(::CreateEventW(nullptr, FALSE, FALSE, nullptr)),
	                          message_thread(utility::CreateThread(nullptr, 0, this, &queue_with_own_thread::message_thread_proc, nullptr, "Message thread", 0, nullptr))
	{
		::WaitForSingleObject(window_created_event, INFINITE);
		::CloseHandle(window_created_event);
	}

	~queue_with_own_thread()
	{
		uninitialize();
	}

	bool safe_to_sync() const
	{
		return gcd::queue::get_current_queue() != *this;
	}

private:
	void uninitialize()
	{
		async([&] {
			::PostQuitMessage(0);
		});
		::WaitForSingleObject(message_thread, INFINITE);
		::CloseHandle(message_thread);
	}

	void register_class()
	{
		::memset(&window_class, 0, sizeof(window_class));
		window_class.cbSize = sizeof(WNDCLASSEXW);
		window_class.hInstance = ::GetModuleHandle(NULL);
		window_class.lpfnWndProc = &queue_with_own_thread::message_proc_helper;
		window_class.cbClsExtra = 0;
		window_class.cbWndExtra = 0;
		window_class.lpszClassName = L"queue_with_own_thread";

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
		set_window(::CreateWindowExW(0, get_class_name(), L"message-only window", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, ::GetModuleHandle(NULL), nullptr));
	}

	HWND get_window() const
	{
		return message_window;
	}

	void set_window(HWND wnd)
	{
		message_window = wnd;
	}

	static LRESULT CALLBACK message_proc_helper(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		return ::DefWindowProcW(wnd, message, wParam, lParam);
	}

	DWORD message_thread_proc(void*)
	{
		steal(gcd::queue::get_current_thread_queue());

		register_class();
		create_window(nullptr);

		MSG msg = {0};
		::PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
		::SetEvent(window_created_event);

		while(BOOL rv = ::GetMessageW(&msg, NULL, 0, 0))
		{
			if(rv == -1)
			{
				dispatch_thread_queue_callback();
				break;
			}

			if(msg.message == dispatch_get_thread_window_message()) {
				dispatch_thread_queue_callback();
				continue;
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

template<typename H, typename S>
struct transition
{
	typedef H handler_type;
	typedef S state_type;

	typedef state_type (handler_type::*state_fun)();
	typedef array<state_type> state_array;
		
	state_fun fun;
	state_array next_states;

	state_type execute(handler_type* hnd) const
	{
		return (hnd->*fun)();
	}
};

template<typename H, typename S, typename E>
struct event_handler : boost::noncopyable
{
	typedef H handler_type;
	typedef S state_type;
	typedef E event_type;

	typedef event_handler<handler_type, state_type, event_type> event_handler_type;

	event_handler(handler_type* handler_) : handler(handler_)
	{
	}

	void send_event(event_type evt)
	{
		return send_event_with_callback(evt, [] {});
	}

	void send_event_with_callback(event_type evt, std::function<void()> f)
	{
		if(my_queue.safe_to_sync()) {
			my_queue.sync([=] {
				event_dispatch(evt);
				f();
			});
		} else {
			dout << "warning: you just attempted to deadlock by recursively making synchronous calls" << std::endl;
			event_dispatch(evt);
			f();
		}
	}

	void post_event(event_type evt)
	{
		return post_event_with_callback(evt, [] {});
	}

	void post_event_with_callback(event_type evt, std::function<void()> f)
	{
		my_queue.async([=] {
			event_dispatch(evt);
			f();
		});
	}

	void send_callback(std::function<void()> f)
	{
		if(my_queue.safe_to_sync()) {
			my_queue.sync([=] {
				f();
			});
		} else {
			dout << "warning: you just attempted to deadlock by recursively making synchronous calls" << std::endl;
			f();
		}
	}

	void post_callback(std::function<void()> f)
	{
		my_queue.async([=] {
			f();
		});
	}

	bool permitted(event_type evt) const
	{
		return handler->get_transition(evt)->fun != nullptr;
	}

private:
	void event_dispatch(event_type evt)
	{
		if(permitted(evt))
		{
			dout << "current state: " << handler->state_name(handler->get_current_state()) << std::endl;
			state_type next_state(handler->get_transition(evt)->execute(handler));
			if(static_cast<int>(next_state) != -1) {
				handler->set_current_state(next_state);
			}
			dout << "new state: " << handler->state_name(handler->get_current_state()) << std::endl;
		}
		else
		{
			dout << "the event " << handler->event_name(evt) << " is not permitted in state " << handler->state_name(handler->get_current_state()) << std::endl;
		}
	}

	handler_type* handler;
	queue_with_own_thread my_queue;
};

#endif
