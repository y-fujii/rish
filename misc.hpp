// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <stdexcept>
#include <algorithm>
#include <vector>
#include <sstream>
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

	inline int stoi( string const& str ) {
		int i;
		if( (istringstream( str ) >> i).fail() ) {
			throw invalid_argument( "" );
		}
		return i;
	}
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

inline int imod( int a, int b ) {
	if( a * b < 0 ) {
		return a % b + b;
	}
	else {
		return a % b;
	}
}

struct ScopeExit {
	template<class T>
	explicit ScopeExit( T const& cb, bool r = true ):
		_callback( cb ), _run( r ) {
	}

	~ScopeExit() {
		if( _run ) {
			try {
				_callback();
			}
			catch( ... ) {}
		}
	}

	private:
		std::function<void ()> const _callback;
		bool const _run;
};

namespace tmeta {
	template<class T0, class T1>
	struct Cons {
		typedef T0 Head;
		typedef T1 Tail;
	};

	struct Null {
	};

	template<class, class> struct Find;

	template<class Tn, class T>
	struct Find<Cons<T, Tn>, T> {
		static int const value = 0;
	};

	template<class T1, class Tn, class T>
	struct Find<Cons<T1, Tn>, T> {
		static int const value = 1 + Find<Tn, T>::value;
	};
}

template<class Tn>
struct Variant {
	int const dTag;

	protected:
		explicit Variant( int t ): dTag( t ) {}
};

template<class, class>
struct VariantImpl;

template<class Tn, class T>
struct VariantImpl<Variant<Tn>, T>: Variant<Tn> {
	static int const sTag = tmeta::Find<Tn, T>::value;

	VariantImpl(): Variant<Tn>( sTag ) {}
};

template<class T>
struct FalseWrapper {
	FalseWrapper( T const& v ): val( v ) {}

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
