// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <algorithm>
#include <vector>
#include <iostream>
#include <tr1/functional>
#include <stdint.h>
#include <cassert>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>


namespace std {
	using namespace tr1;

	template<class SrcIter, class DstIter, class Pred>
	inline DstIter copy_if( SrcIter srcIt, SrcIter srcEnd, DstIter dstIt, Pred pred ) {
		while( srcIt != srcEnd ) {
			if( pred( *srcIt ) ) {
				*dstIt++ = *srcIt;
			}
			++srcIt;
		}
		return dstIt;
	}

	template<class Iter, class Func>
	inline bool any_of( Iter bgn, Iter end, Func f ) {
		return find_if( bgn, end, f ) != end;
	}

	template<class Iter, class Func>
	inline bool all_of( Iter bgn, Iter end, Func f ) {
		return find_if( bgn, end, not1( f ) ) == end;
	}

	template<class T>
	struct atomic {
		explicit atomic( T const& v ): _val( v ) {
		}

		T load() volatile {
			__sync_synchronize();
			T volatile v = _val;
			__sync_synchronize();
			return v;
		}

		void store( T const& v ) volatile {
			__sync_synchronize();
			_val = v;
			__sync_synchronize();
		}

		private:
			T volatile _val;
	};
}

/* this does not work on gcc-4.5.x
struct {
	template<class T>
	operator T*() const {
		return 0;
	}
} const nullptr = {};
*/
#define nullptr (0)

template<class T, unsigned N>
unsigned size( T const (&)[N] ) {
	return N;
}

struct ScopeExit {
	template<class T>
	explicit ScopeExit( T const& cb, bool r = true ):
		_callback( cb ), _run( r ) {
	}

	~ScopeExit() {
		if( _run ) {
			_callback();
		}
	}

	private:
		std::function<void ()> const _callback;
		bool const _run;
};

template<class T>
struct VariantBase {
	int const dTag;

	protected:
		VariantBase(): dTag( -1 ) {
		}
};

template<class T, int Tag>
struct Variant: T {
	static int const sTag = Tag;

	Variant() {
		const_cast<int&>( VariantBase<T>::dTag ) = Tag;
	}
};

template<class T>
struct FalseWrapper {
	FalseWrapper( T const& v ): val( v ) {
	}

	operator bool() const {
		return false;
	}

	T val;
};

#define VARIANT_SWITCH( type, rhs ) \
	if( FalseWrapper<type*> _variant_switch_value_ = (rhs) ) {} else \
	switch( _variant_switch_value_.val->dTag ) {

#define VARIANT_CASE( type, lhs ) \
		break; \
	} \
	case type::sTag: { \
		type* lhs = static_cast<type*>( _variant_switch_value_.val ); \
		(void)(lhs);

#define VARIANT_DEFAULT \
		break; \
	} \
	default:

struct IOError: std::exception {
};
