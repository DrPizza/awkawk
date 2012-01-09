//  Copyright (C) 2011 Peter Bright
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

#ifndef SCOPEGUARD_HPP
#define SCOPEGUARD_HPP

#include <utility>

namespace util
{
	struct scope_guard_base
	{
		void dismiss() const
		{
			active = false;
		}
	protected:
		explicit scope_guard_base(bool active_ = true) : active(active_)
		{
		}

		~scope_guard_base()
		{
		}

		mutable bool active;
	private:
		scope_guard_base(const scope_guard_base&);
		scope_guard_base& operator=(const scope_guard_base&);
	};

	template<typename F>
	struct scope_guard_with_functor : scope_guard_base
	{
		explicit scope_guard_with_functor(F f) : func(std::move(f))
		{
		}

		// in an ideal world, this would be a move constructor, dismiss would be non-const, and active would be non-mutable
		// sadly, http://connect.microsoft.com/VisualStudio/feedback/details/571550/the-copy-constructor-incorrectly-chosen-for-an-rvalue and
		// https://connect.microsoft.com/VisualStudio/feedback/details/553184/non-copyable-rvalues-cannot-be-bound-to-rvalue-references seem
		// to cause problems. Even though one is "fixed" and one is "won't fix"
		scope_guard_with_functor(const scope_guard_with_functor& rhs) : scope_guard_base(rhs.active), func(std::move(rhs.func))
		{
			rhs.dismiss();
		}

		~scope_guard_with_functor()
		{
			if(active)
			{
				func();
			}
		}

	private:
		F func;
		scope_guard_with_functor& operator=(const scope_guard_with_functor<F>&);
	};

	template<typename F>
	scope_guard_with_functor<F> make_guard(F f)
	{
		return scope_guard_with_functor<F>(std::move(f));
	}

	typedef scope_guard_base&& scope_guard;
}

#define CONCATENATE_DIRECT(s1, s2)  s1##s2
#define CONCATENATE(s1, s2)         CONCATENATE_DIRECT(s1, s2)
#define ANONYMOUS_VARIABLE(str)     CONCATENATE(str, __LINE__)

#define ON_BLOCK_EXIT(lambda) util::scope_guard ANONYMOUS_VARIABLE(scope_guard) = util::make_guard([&]{lambda;})

#endif
