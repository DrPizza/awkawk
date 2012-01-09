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

#pragma once

#ifndef UTIL__H
#define UTIL__H

#include "stdafx.h"

#include "utility/locking_stream.hpp"
#include "loki/singleton.h"

#define FAIL_THROW(x) do { HRESULT CREATE_NAME(hr); if(FAILED(CREATE_NAME(hr) = ( x ))) { _com_raise_error(CREATE_NAME(hr)); } } while(0)
#define FAIL_RET(x) do { HRESULT CREATE_NAME(hr); if(FAILED(CREATE_NAME(hr) = ( x ))) { return CREATE_NAME(hr); } } while(0)
#define CREATE_NAME(name) CREATE_NAME_IND(name, __LINE__)
#define CREATE_NAME_IND(first, second) PASTE_2(first, second)
#define PASTE_2(a, b) a ## b

namespace
{
	struct DebugStreamsImpl
	{
		utility::locking_ostream locked_dout;
		utility::wlocking_ostream locked_wdout;
#ifdef DEBUG
		DebugStreamsImpl() : locked_dout(std::cout), locked_wdout(std::wcout)
		//DebugStreamsImpl() : locked_dout(utility::DebugStreams::Instance().dout), locked_wdout(utility::DebugStreams::Instance().wdout)
#else
		DebugStreamsImpl() : locked_dout(utility::DebugStreams::Instance().dout), locked_wdout(utility::DebugStreams::Instance().wdout)
#endif
		{
		}
	};
}

// notice that I rely on the CRT to perform initialization of statics in a safe manner.  Using a thread-safe policy crashes during static initialization.
typedef Loki::SingletonHolder<DebugStreamsImpl, Loki::CreateUsingNew, Loki::DefaultLifetime, Loki::SingleThreaded> DebugStreams;

extern utility::locking_ostream& dout;
extern utility::wlocking_ostream& wdout;
extern utility::locking_ostream& derr;
extern utility::wlocking_ostream& wderr;

template<typename T>
inline std::basic_ostream<T>& operator<<(std::basic_ostream<T>& lhs, const GUID& rhs)
{
	long flags(lhs.flags());
	std::streamsize width(lhs.width());
	T fill(lhs.fill());

	lhs << std::hex << std::setfill(T('0')) ;
	lhs << std::setw(8) << rhs.Data1 << T('-')
	    << std::setw(4) << rhs.Data2 << T('-') << rhs.Data3 << T('-');
	lhs << std::setw(2) << static_cast<unsigned long>(rhs.Data4[0]) << std::setw(2) << static_cast<unsigned long>(rhs.Data4[1]) << T('-');

	for(int i(2); i < 8; ++i)
	{
		lhs << std::setw(2) << static_cast<unsigned long>(rhs.Data4[i]);
	}

	lhs.flags(flags);
	lhs.width(width);
	lhs.fill(fill);
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

struct direct3d_manager;

struct direct3d_object {
	direct3d_object(direct3d_manager* manager);
	virtual ~direct3d_object();

	// create D3DPOOL_MANAGED resources
	HRESULT on_device_created(IDirect3DDevice9Ptr new_device) {
		return do_on_device_created(new_device);
	}

	// create D3DPOOL_DEFAULT resources
	HRESULT on_device_reset() {
		return do_on_device_reset();
	}

	// destroy D3DPOOL_DEFAULT resources
	HRESULT on_device_lost() {
		return do_on_device_lost();
	}

	// destroy D3DPOOL_MANAGED resources
	void on_device_destroyed() {
		on_device_lost();
		do_on_device_destroyed();
	}

protected:
	// create D3DPOOL_MANAGED resources
	virtual HRESULT do_on_device_created(IDirect3DDevice9Ptr new_device) = 0;
	// create D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_reset() = 0;
	// destroy D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_lost() = 0;
	// destroy D3DPOOL_MANAGED resources
	virtual void do_on_device_destroyed() = 0;

private:
	direct3d_manager* manager;
};

struct direct3d_renderable : direct3d_object {
	direct3d_renderable(direct3d_manager* manager_) : direct3d_object(manager_) {
	}

	// called by a renderer between BegineScene/EndScene pair
	void emit_scene() {
		return do_emit_scene();
	}

protected:
	virtual void do_emit_scene() = 0;
};

struct direct3d_manager : direct3d_renderable {
	direct3d_manager(direct3d_manager* manager_) : direct3d_renderable(manager_),
	                                               cs("direct3d_manager") {
	}

	void enregister(direct3d_object* o) {
		LOCK(cs);
		objects.push_back(o);
	}

	void deregister(direct3d_object* o) {
		LOCK(cs);
		std::list<direct3d_object*>::iterator pos(std::find(objects.begin(), objects.end(), o));
		if(pos != objects.end()) {
			objects.erase(pos);
		}
	}

protected:
	// create D3DPOOL_MANAGED resources
	virtual HRESULT do_on_device_created(IDirect3DDevice9Ptr new_device) {
		LOCK(cs);
		std::for_each(objects.begin(), objects.end(), [=](direct3d_object* o) {
			o->on_device_created(new_device);
		});
		return S_OK;
	}
	// create D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_reset() {
		LOCK(cs);
		std::for_each(objects.begin(), objects.end(), [=](direct3d_object* o) {
			o->on_device_reset();
		});
		return S_OK;
	}
	// destroy D3DPOOL_DEFAULT resources
	virtual HRESULT do_on_device_lost() {
		LOCK(cs);
		std::for_each(objects.begin(), objects.end(), [=](direct3d_object* o) {
			o->on_device_lost();
		});
		return S_OK;
	}
	// destroy D3DPOOL_MANAGED resources
	virtual void do_on_device_destroyed() {
		LOCK(cs);
		std::for_each(objects.begin(), objects.end(), [=](direct3d_object* o) {
			o->on_device_destroyed();
		});
	}

	virtual void do_emit_scene() {
		LOCK(cs);
		std::for_each(objects.begin(), objects.end(), [=](direct3d_object* o) {
			if(direct3d_renderable* r = dynamic_cast<direct3d_renderable*>(o)) {
				r->emit_scene();
			}
		});
	}

	std::list<direct3d_object*> objects;

private:
	utility::critical_section cs;
};

inline direct3d_object::direct3d_object(direct3d_manager* manager_) : manager(manager_) {
	if(manager) {
		manager->enregister(this);
	}
}

inline direct3d_object::~direct3d_object() {
	if(manager) {
		manager->deregister(this);
	}
}


inline std::string format_name(D3DFORMAT format) {
	switch(format) {
	case D3DFMT_UNKNOWN: return "D3DFMT_UNKNOWN"; break;
	case D3DFMT_R8G8B8: return "D3DFMT_R8G8B8"; break;
	case D3DFMT_A8R8G8B8: return "D3DFMT_A8R8G8B8"; break;
	case D3DFMT_X8R8G8B8: return "D3DFMT_X8R8G8B8"; break;
	case D3DFMT_R5G6B5: return "D3DFMT_R5G6B5"; break;
	case D3DFMT_X1R5G5B5: return "D3DFMT_X1R5G5B5"; break;
	case D3DFMT_A1R5G5B5: return "D3DFMT_A1R5G5B5"; break;
	case D3DFMT_A4R4G4B4: return "D3DFMT_A4R4G4B4"; break;
	case D3DFMT_R3G3B2: return "D3DFMT_R3G3B2"; break;
	case D3DFMT_A8: return "D3DFMT_A8"; break;
	case D3DFMT_A8R3G3B2: return "D3DFMT_A8R3G3B2"; break;
	case D3DFMT_X4R4G4B4: return "D3DFMT_X4R4G4B4"; break;
	case D3DFMT_A2B10G10R10: return "D3DFMT_A2B10G10R10"; break;
	case D3DFMT_A8B8G8R8: return "D3DFMT_A8B8G8R8"; break;
	case D3DFMT_X8B8G8R8: return "D3DFMT_X8B8G8R8"; break;
	case D3DFMT_G16R16: return "D3DFMT_G16R16"; break;
	case D3DFMT_A2R10G10B10: return "D3DFMT_A2R10G10B10"; break;
	case D3DFMT_A16B16G16R16: return "D3DFMT_A16B16G16R16"; break;
	case D3DFMT_A8P8: return "D3DFMT_A8P8"; break;
	case D3DFMT_P8: return "D3DFMT_P8"; break;
	case D3DFMT_L8: return "D3DFMT_L8"; break;
	case D3DFMT_A8L8: return "D3DFMT_A8L8"; break;
	case D3DFMT_A4L4: return "D3DFMT_A4L4"; break;
	case D3DFMT_V8U8: return "D3DFMT_V8U8"; break;
	case D3DFMT_L6V5U5: return "D3DFMT_L6V5U5"; break;
	case D3DFMT_X8L8V8U8: return "D3DFMT_X8L8V8U8"; break;
	case D3DFMT_Q8W8V8U8: return "D3DFMT_Q8W8V8U8"; break;
	case D3DFMT_V16U16: return "D3DFMT_V16U16"; break;
	case D3DFMT_A2W10V10U10: return "D3DFMT_A2W10V10U10"; break;
	case D3DFMT_UYVY: return "D3DFMT_UYVY"; break;
	case D3DFMT_R8G8_B8G8: return "D3DFMT_R8G8_B8G8"; break;
	case D3DFMT_YUY2: return "D3DFMT_YUY2"; break;
	case D3DFMT_G8R8_G8B8: return "D3DFMT_G8R8_G8B8"; break;
	case D3DFMT_DXT1: return "D3DFMT_DXT1"; break;
	case D3DFMT_DXT2: return "D3DFMT_DXT2"; break;
	case D3DFMT_DXT3: return "D3DFMT_DXT3"; break;
	case D3DFMT_DXT4: return "D3DFMT_DXT4"; break;
	case D3DFMT_DXT5: return "D3DFMT_DXT5"; break;
	case D3DFMT_D16_LOCKABLE: return "D3DFMT_D16_LOCKABLE"; break;
	case D3DFMT_D32: return "D3DFMT_D32"; break;
	case D3DFMT_D15S1: return "D3DFMT_D15S1"; break;
	case D3DFMT_D24S8: return "D3DFMT_D24S8"; break;
	case D3DFMT_D24X8: return "D3DFMT_D24X8"; break;
	case D3DFMT_D24X4S4: return "D3DFMT_D24X4S4"; break;
	case D3DFMT_D16: return "D3DFMT_D16"; break;
	case D3DFMT_D32F_LOCKABLE: return "D3DFMT_D32F_LOCKABLE"; break;
	case D3DFMT_D24FS8: return "D3DFMT_D24FS8"; break;
	case D3DFMT_D32_LOCKABLE: return "D3DFMT_D32_LOCKABLE"; break;
	case D3DFMT_S8_LOCKABLE: return "D3DFMT_S8_LOCKABLE"; break;
	case D3DFMT_L16: return "D3DFMT_L16"; break;
	case D3DFMT_VERTEXDATA: return "D3DFMT_VERTEXDATA"; break;
	case D3DFMT_INDEX16: return "D3DFMT_INDEX16"; break;
	case D3DFMT_INDEX32: return "D3DFMT_INDEX32"; break;
	case D3DFMT_Q16W16V16U16: return "D3DFMT_Q16W16V16U16"; break;
	case D3DFMT_MULTI2_ARGB8: return "D3DFMT_MULTI2_ARGB8"; break;
	case D3DFMT_R16F: return "D3DFMT_R16F"; break;
	case D3DFMT_G16R16F: return "D3DFMT_G16R16F"; break;
	case D3DFMT_A16B16G16R16F: return "D3DFMT_A16B16G16R16F"; break;
	case D3DFMT_R32F: return "D3DFMT_R32F"; break;
	case D3DFMT_G32R32F: return "D3DFMT_G32R32F"; break;
	case D3DFMT_A32B32G32R32F: return "D3DFMT_A32B32G32R32F"; break;
	case D3DFMT_CxV8U8: return "D3DFMT_CxV8U8"; break;
	case D3DFMT_A1: return "D3DFMT_A1"; break;
	case D3DFMT_A2B10G10R10_XR_BIAS: return "D3DFMT_A2B10G10R10_XR_BIAS"; break;
	case D3DFMT_BINARYBUFFER: return "D3DFMT_BINARYBUFFER"; break;
	default: {
			if(format > '0000') {
				char tmp[5];
				memcpy(tmp, &format, 4);
				tmp[4] = '\0';
				return tmp;
			} else {
				std::stringstream ss;
				ss << "unknown format: " << std::hex << format;
				return ss.str();
			}
		}
	}
}

inline std::string resource_name(D3DRESOURCETYPE res) {
	switch(res) {
	case D3DRTYPE_SURFACE: return "D3DRTYPE_SURFACE"; break;
	case D3DRTYPE_VOLUME: return "D3DRTYPE_VOLUME"; break;
	case D3DRTYPE_TEXTURE: return "D3DRTYPE_TEXTURE"; break;
	case D3DRTYPE_VOLUMETEXTURE: return "D3DRTYPE_VOLUMETEXTURE"; break;
	case D3DRTYPE_CUBETEXTURE: return "D3DRTYPE_CUBETEXTURE"; break;
	case D3DRTYPE_VERTEXBUFFER: return "D3DRTYPE_VERTEXBUFFER"; break;
	case D3DRTYPE_INDEXBUFFER: return "D3DRTYPE_INDEXBUFFER"; break;
	default:
		return "unknown type";
	}
}

inline std::string usage_name(DWORD usage) {
	std::string result(" | ");
	if(usage & D3DUSAGE_RENDERTARGET) {
		result += "D3DUSAGE_RENDERTARGET | ";
	}
	if(usage & D3DUSAGE_DEPTHSTENCIL) {
		result += "D3DUSAGE_DEPTHSTENCIL | ";
	}
	if(usage & D3DUSAGE_DYNAMIC) {
		result += "D3DUSAGE_DYNAMIC | ";
	}
	if(usage & D3DUSAGE_NONSECURE) {
		result += "D3DUSAGE_NONSECURE | ";
	}
	if(usage & D3DUSAGE_AUTOGENMIPMAP) {
		result += "D3DUSAGE_AUTOGENMIPMAP | ";
	}
	if(usage & D3DUSAGE_DMAP) {
		result += "D3DUSAGE_DMAP | ";
	}
	if(usage & D3DUSAGE_QUERY_LEGACYBUMPMAP) {
		result += "D3DUSAGE_QUERY_LEGACYBUMPMAP | ";
	}
	if(usage & D3DUSAGE_QUERY_SRGBREAD) {
		result += "D3DUSAGE_QUERY_SRGBREAD | ";
	}
	if(usage & D3DUSAGE_QUERY_FILTER) {
		result += "D3DUSAGE_QUERY_FILTER | ";
	}
	if(usage & D3DUSAGE_QUERY_SRGBWRITE) {
		result += "D3DUSAGE_QUERY_SRGBWRITE | ";
	}
	if(usage & D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING) {
		result += "D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING | ";
	}
	if(usage & D3DUSAGE_QUERY_VERTEXTEXTURE) {
		result += "D3DUSAGE_QUERY_VERTEXTEXTURE | ";
	}
	if(usage & D3DUSAGE_QUERY_WRAPANDMIP) {
		result += "D3DUSAGE_QUERY_WRAPANDMIP | ";
	}
	if(usage & D3DUSAGE_WRITEONLY) {
		result += "D3DUSAGE_WRITEONLY | ";
	}
	if(usage & D3DUSAGE_SOFTWAREPROCESSING) {
		result += "D3DUSAGE_SOFTWAREPROCESSING | ";
	}
	if(usage & D3DUSAGE_DONOTCLIP) {
		result += "D3DUSAGE_DONOTCLIP | ";
	}
	if(usage & D3DUSAGE_POINTS) {
		result += "D3DUSAGE_POINTS | ";
	}
	if(usage & D3DUSAGE_RTPATCHES) {
		result += "D3DUSAGE_RTPATCHES | ";
	}
	if(usage & D3DUSAGE_NPATCHES) {
		result += "D3DUSAGE_NPATCHES | ";
	}
	if(usage & D3DUSAGE_TEXTAPI) {
		result += "D3DUSAGE_TEXTAPI | ";
	}
	if(usage & D3DUSAGE_RESTRICTED_CONTENT) {
		result += "D3DUSAGE_RESTRICTED_CONTENT | ";
	}
	if(usage & D3DUSAGE_RESTRICT_SHARED_RESOURCE) {
		result += "D3DUSAGE_RESTRICT_SHARED_RESOURCE | ";
	}
	if(usage & D3DUSAGE_RESTRICT_SHARED_RESOURCE_DRIVER) {
		result += "D3DUSAGE_RESTRICT_SHARED_RESOURCE_DRIVER | ";
	}
	return result.substr(0, result.size() - 3);
}

#pragma warning(push)
#pragma warning(disable: 4063)
inline std::string pool_name(D3DPOOL pool) {
	switch(pool) {
	case D3DPOOL_DEFAULT: return "D3DPOOL_DEFAULT"; break;
	case D3DPOOL_MANAGED: return "D3DPOOL_MANAGED"; break;
	case D3DPOOL_SYSTEMMEM: return "D3DPOOL_SYSTEMMEM"; break;
	case D3DPOOL_SCRATCH: return "D3DPOOL_SCRATCH"; break;
	// the following values are found in the DXVA header, but aren't visible if D3D is also included
	case 4: return "D3DPOOL_LOCALVIDMEM"; break;
	case 5: return "D3DPOOL_NONLOCALVIDMEM"; break;
	default:
		{
			std::stringstream ss;
			ss << "unknown pool: " << std::hex << pool;
			return ss.str();
		}
	}
}
#pragma warning(pop)

inline std::string multisample_name(D3DMULTISAMPLE_TYPE ms) {
	switch(ms) {
	case D3DMULTISAMPLE_NONE: return "D3DMULTISAMPLE_NONE"; break;
	case D3DMULTISAMPLE_NONMASKABLE: return "D3DMULTISAMPLE_NONMASKABLE"; break;
	case D3DMULTISAMPLE_2_SAMPLES: return "D3DMULTISAMPLE_2_SAMPLES"; break;
	case D3DMULTISAMPLE_3_SAMPLES: return "D3DMULTISAMPLE_3_SAMPLES"; break;
	case D3DMULTISAMPLE_4_SAMPLES: return "D3DMULTISAMPLE_4_SAMPLES"; break;
	case D3DMULTISAMPLE_5_SAMPLES: return "D3DMULTISAMPLE_5_SAMPLES"; break;
	case D3DMULTISAMPLE_6_SAMPLES: return "D3DMULTISAMPLE_6_SAMPLES"; break;
	case D3DMULTISAMPLE_7_SAMPLES: return "D3DMULTISAMPLE_7_SAMPLES"; break;
	case D3DMULTISAMPLE_8_SAMPLES: return "D3DMULTISAMPLE_8_SAMPLES"; break;
	case D3DMULTISAMPLE_9_SAMPLES: return "D3DMULTISAMPLE_9_SAMPLES"; break;
	case D3DMULTISAMPLE_10_SAMPLES: return "D3DMULTISAMPLE_10_SAMPLES"; break;
	case D3DMULTISAMPLE_11_SAMPLES: return "D3DMULTISAMPLE_11_SAMPLES"; break;
	case D3DMULTISAMPLE_12_SAMPLES: return "D3DMULTISAMPLE_12_SAMPLES"; break;
	case D3DMULTISAMPLE_13_SAMPLES: return "D3DMULTISAMPLE_13_SAMPLES"; break;
	case D3DMULTISAMPLE_14_SAMPLES: return "D3DMULTISAMPLE_14_SAMPLES"; break;
	case D3DMULTISAMPLE_15_SAMPLES: return "D3DMULTISAMPLE_15_SAMPLES"; break;
	case D3DMULTISAMPLE_16_SAMPLES: return "D3DMULTISAMPLE_16_SAMPLES"; break;
	default:
		{
			std::stringstream ss;
			ss << "unknown multisample: " << std::hex << ms;
			return ss.str();
		}
	}
}

inline void print_surface_desc(D3DSURFACE_DESC desc) {
	dout << "\tsize                         : " << desc.Width << " x " << desc.Height << std::endl
	     << "\tdelivered format             : " << format_name(desc.Format) << std::endl
	     << "\tdelivered resource           : " << resource_name(desc.Type) << std::endl
	     << "\tdelivered usage              : " << usage_name(desc.Usage) << std::endl
	     << "\tdelivered pool               : " << pool_name(desc.Pool) << std::endl
	     << "\tdelivered multisample type   : " << multisample_name(desc.MultiSampleType) << std::endl
	     << "\tdelivered multisample quality: " << std::hex << desc.MultiSampleQuality << std::dec << std::endl;
}

inline IDirect3DTexture9Ptr load_texture_from_resource(IDirect3DDevice9Ptr device, int resource_id, D3DXIMAGE_INFO* info)
{
	HRSRC res(::FindResourceW(NULL, MAKEINTRESOURCEW(resource_id), L"PNG"));
	DWORD size(::SizeofResource(::GetModuleHandle(NULL), res));
	HGLOBAL glob(::LoadResource(::GetModuleHandle(NULL), res));
	void* data(::LockResource(glob));
	IDirect3DTexture9Ptr texture;
	FAIL_THROW(D3DXCreateTextureFromFileInMemoryEx(device, data, size, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_FROM_FILE, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, info, nullptr, &texture));
	return texture;
}

template<typename T>
struct array
{
	array() : length(0), raw(new T[length])
	{
	}
	array(size_t length_) : length(length_), raw(new T[length])
	{
	}
	array(size_t length_, const T& t) : length(length_), raw(new T[length])
	{
		for(size_t i(0); i < length; ++i)
		{
			raw[i] = t;
		}
	}
	~array()
	{
		delete [] raw;
	}
	array(const array<T>& rhs) : length(rhs.length), raw(new T[length])
	{
		for(size_t i(0); i < length; ++i)
		{
			raw[i] = rhs.raw[i];
		}
	}
	// this is pretty gross, but absent array literals, it's the best I can do
	array<T> operator<<(const T& t)
	{
		array<T> rv(length + 1);
		for(size_t i(0); i < length; ++i)
		{
			rv[i] = (*this)[i];
		}
		rv[length] = t;
		return rv;
	}
	const size_t length;
	const T& operator[](size_t idx) const { return raw[idx]; }
	      T& operator[](size_t idx)       { return raw[idx]; }
private:
	T* raw;
};

template <typename T, size_t N>
struct vector
{
	typedef vector<T, N> my_type;
	typedef T* iterator;
	typedef const T* const_iterator;

	vector()
	{
		for(size_t i(0); i < N; ++i)
		{
			elems[i] = T();
		}
	}

	iterator begin()
	{
		return elems;
	}

	const_iterator begin() const
	{
		return elems;
	}

	iterator end()
	{
		return elems + N;
	}

	const_iterator end() const
	{
		return elems + N;
	}

	size_t size() const
	{
		return N;
	}

	T length() const
	{
		T l = T();
		for(size_t i(0); i < N; ++i)
		{
			l += elems[i] * elems[i];
		}
		return std::sqrt(l);
	}

	my_type normalize() const
	{
		my_type rv;
		T len(length());
		if(len == T())
		{
			return rv;
		}
		for(size_t i(0); i < N; ++i)
		{
			rv.elems[i] = elems[i] / len;
		}
		return rv;
	}

	my_type& operator+()
	{
		for(size_t i(0); i < N; ++i)
		{
			elems[i] = +elems[i];
		}
		return *this;
	}

	my_type& operator-()
	{
		for(size_t i(0); i < N; ++i)
		{
			elems[i] = -elems[i];
		}
		return *this;
	}

	my_type& operator+=(const my_type& rhs)
	{
		for(size_t i(0); i < N; ++i)
		{
			elems[i] += rhs.elems[i];
		}
		return *this;
	}

	my_type& operator-=(const my_type& rhs)
	{
		for(size_t i(0); i < N; ++i)
		{
			elems[i] -= rhs.elems[i];
		}
		return *this;
	}

	my_type& operator*=(const my_type& rhs)
	{
		for(size_t i(0); i < N; ++i)
		{
			elems[i] *= rhs.elems[i];
		}
		return *this;
	}

	my_type& operator*=(T rhs)
	{
		for(size_t i(0); i < N; ++i)
		{
			elems[i] *= rhs;
		}
		return *this;
	}

	my_type& operator/=(const my_type& rhs)
	{
		for(size_t i(0); i < N; ++i)
		{
			elems[i] /= rhs.elems[i];
		}
		return *this;
	}

	my_type& operator/=(T rhs)
	{
		for(size_t i(0); i < N; ++i)
		{
			elems[i] /= rhs;
		}
		return *this;
	}

	T& operator[](size_t idx)
	{
		return elems[idx];
	}

	const T& operator[](size_t idx) const
	{
		return elems[idx];
	}

private:
	T elems[N];
};

template<typename C, typename T, size_t N>
inline std::basic_ostream<C>& operator<<(std::basic_ostream<C>& os, const vector<T, N>& rhs)
{
	os << C('{');
	os << rhs[0];
	for(size_t idx(1); idx < N; ++idx)
	{
		os << C(',');
		os << C(' ');
		os << rhs[idx];
	}
	os << C('}');
	return os;
}

template<typename T>
vector<T, 2> normal(const vector<T, 2>& rhs)
{
	vector<T, 2> rv;
	rv[0] = rhs[1];
	rv[1] = rhs[0];
	return rv;
}

template<typename T>
vector<T, 3> cross(const vector<T, 3>& lhs, const vector<T, 3>& rhs)
{
	vector<T, 3> rv;
	rv[0] = (lhs[1] * rhs[2]) - (lhs[2] * rhs[1]);
	rv[1] = (lhs[0] * rhs[2]) - (lhs[0] * rhs[2]);
	rv[2] = (lhs[0] * rhs[1]) - (lhs[0] * rhs[1]);
	return rv;
}

template<typename T, size_t N>
vector<T, N> operator+(const vector<T, N>& lhs, const vector<T, N>& rhs)
{
	vector<T, N> rv(lhs);
	rv += rhs;
	return rv;
}

template<typename T, size_t N>
vector<T, N> operator-(const vector<T, N>& lhs, const vector<T, N>& rhs)
{
	vector<T, N> rv(lhs);
	rv -= rhs;
	return rv;
}

template<typename T, size_t N>
vector<T, N> operator*(const vector<T, N>& lhs, const vector<T, N>& rhs)
{
	vector<T, N> rv(lhs);
	rv *= rhs;
	return rv;
}

template<typename T, size_t N>
vector<T, N> operator*(const vector<T, N>& lhs, T rhs)
{
	vector<T, N> rv(lhs);
	rv *= rhs;
	return rv;
}

template<typename T, size_t N>
vector<T, N> operator*(T lhs, const vector<T, N>& rhs)
{
	vector<T, N> rv(rhs);
	rv *= lhs;
	return rv;
}

template<typename T, size_t N>
vector<T, N> operator/(const vector<T, N>& lhs, const vector<T, N>& rhs)
{
	vector<T, N> rv(lhs);
	rv /= rhs;
	return rv;
}

template<typename T, size_t N>
vector<T, N> operator/(const vector<T, N>& lhs, T rhs)
{
	vector<T, N> rv(lhs);
	rv /= rhs;
	return rv;
}

template<typename T>
T cos_angle(const vector<T, 2>& lhs, const vector<T, 2>& rhs)
{
	T divisor(lhs.length() * rhs.length());
	if(divisor == T())
	{
		return divisor;
	}
	else
	{
		vector<T, 2> products(lhs * rhs);
		return std::accumulate(products.begin(), products.end(), T()) / divisor;
	}
}

typedef vector<double, 2> vec2d;
typedef vector<float, 2> vec2f;
typedef vector<double, 3> vec3d;
typedef vector<float, 3> vec3f;
typedef vector<float, 4> vec4f;

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

inline D3DXMATRIX* clamp_to_zero(D3DXMATRIX* m, float threshold)
{
	for(size_t i(0); i < 4; ++i)
	{
		for(size_t j(0); j < 4; ++j)
		{
			if(abs(m->m[i][j]) < threshold)
			{
				m->m[i][j] = 0.0f;
			}
		}
	}
	return m;
}

template<typename T>
inline std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const D3DXMATRIX& m)
{
	return os << m._11 << T(' ') << m._12 << T(' ') << m._13 << T(' ') << m._14 << T('\n')
	          << m._21 << T(' ') << m._22 << T(' ') << m._23 << T(' ') << m._24 << T('\n')
	          << m._31 << T(' ') << m._32 << T(' ') << m._33 << T(' ') << m._34 << T('\n')
	          << m._41 << T(' ') << m._42 << T(' ') << m._43 << T(' ') << m._44;
}

typedef boost::rational<__int32> rational_type;

#endif
