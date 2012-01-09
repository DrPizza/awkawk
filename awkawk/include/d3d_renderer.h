//  Copyright (C) 2012 Peter Bright
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

#ifndef D3D_RENDERER__H
#define D3D_RENDERER__H

#include "util.h"
#include "direct3d.h"

#include "utility/interlocked_containers.hpp"

_COM_SMARTPTR_TYPEDEF(IDirect3DDeviceManager9, __uuidof(IDirect3DDeviceManager9));

struct awkawk;
struct player_direct_show;
struct shared_texture_queue;

struct d3d_renderer : direct3d_manager, boost::noncopyable {
	d3d_renderer(awkawk* player_, shared_texture_queue* texture_queue_, HWND window_);
	~d3d_renderer();

	void start_rendering();
	void stop_rendering();

	void signal_new_frame() const {
		::SetEvent(render_event);
	}

	void signal_new_frame_and_wait() const {
		::SignalObjectAndWait(render_and_wait_event, render_finished_event, INFINITE, FALSE);
	}

	void set_render_fps(unsigned int fps_)
	{
		LOCK(cs);
		fps = clamp(fps_, 25u, 60u);
	}

	void schedule_render() const
	{
		schedule_render(1.0f);
	}

	void schedule_render(float frames_ahead) const
	{
		LARGE_INTEGER dueTime = { 0 };
		dueTime.QuadPart = static_cast<LONGLONG>(frames_ahead * (-10000000.0f / static_cast<float>(get_render_fps())));
		::SetWaitableTimer(render_timer, &dueTime, 0, nullptr, nullptr, FALSE);
	}

	unsigned int get_render_fps() const
	{
		return fps;
	}

	IDirect3DDevice9ExPtr get_device()
	{
		return device;
	}

protected:
	// create D3DPOOL_MANAGED resources
	virtual HRESULT do_on_device_created(IDirect3DDevice9Ptr new_device) {
		return direct3d_manager::do_on_device_created(new_device);
	}

	// create D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_reset() {
		return direct3d_manager::do_on_device_reset();
	}

	// destroy D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_lost()
	{
		direct3d_manager::do_on_device_lost();
		// I need to do more than this, I think.
		return S_OK;
	}

	// destroy D3DPOOL_MANAGED resources
	virtual void do_on_device_destroyed()
	{
		direct3d_manager::do_on_device_destroyed();
		on_device_lost();
	}

	virtual void do_emit_scene();

private:
	mutable utility::critical_section cs;

	awkawk* player;
	shared_texture_queue* texture_queue;
	HWND window;

	// D3D methods
	void create_d3d();
	void destroy_d3d();
	void create_device();
	void destroy_device();
	void reset_device();
	bool needs_display_change() const;
	void reset();

	void render();

	// direct3d gubbins
	std::unique_ptr<direct3d9> d3d9;
	UINT reset_token;
	IDirect3DDeviceManager9Ptr device_manager;

	IDirect3DDevice9ExPtr device;

	// rendering
	HANDLE render_timer;
	HANDLE render_event;
	HANDLE render_and_wait_event;
	HANDLE render_finished_event;
	HANDLE cancel_render;
	HANDLE render_thread;
	DWORD render_thread_proc(void*);
	unsigned int fps;

	D3DPRESENT_PARAMETERS presentation_parameters;
	void set_device_state();
};

#endif
