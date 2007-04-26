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
#define FAIL_RET(x) do { HRESULT CREATE_NAME(hr); if(FAILED(CREATE_NAME(hr) = ( x ))) { return CREATE_NAME(hr); } } while(0)
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

struct device_loss_handler
{
	virtual void begin_device_loss() = 0;
	virtual void end_device_loss(IDirect3DDevice9Ptr) = 0;
};

struct direct3d_object
{
	//virtual HRESULT on_device_change() = 0;
	// create D3DPOOL_MANAGED resources
	virtual HRESULT on_device_created(IDirect3DDevice9Ptr new_device) = 0;
	// create D3DPOOL_DEFAULT resources
	virtual HRESULT on_device_reset() = 0;
	// destroy D3DPOOL_DEFAULT resources
	virtual HRESULT on_device_lost() = 0;
	// destroy D3DPOOL_DEFAULT resources
	virtual void on_device_destroyed() = 0;
};

inline IDirect3DTexture9Ptr load_texture_from_resource(IDirect3DDevice9Ptr device, int resource_id, D3DXIMAGE_INFO* info)
{
	HRSRC res(::FindResourceW(NULL, MAKEINTRESOURCEW(resource_id), L"PNG"));
	DWORD size(::SizeofResource(::GetModuleHandle(NULL), res));
	HGLOBAL glob(::LoadResource(::GetModuleHandle(NULL), res));
	void* data(::LockResource(glob));
	IDirect3DTexture9Ptr texture;
	FAIL_THROW(D3DXCreateTextureFromFileInMemoryEx(device, data, size, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 1, D3DFMT_FROM_FILE, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, info, NULL, &texture));
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

inline void free_media_type(AM_MEDIA_TYPE& mt)
{
	if(mt.cbFormat != 0)
	{
		::CoTaskMemFree(mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if(mt.pUnk != NULL)
	{
		// Unecessary because pUnk should not be used, but safest.
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}


#endif
