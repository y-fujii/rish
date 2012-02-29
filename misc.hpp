// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <string>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <sstream>
#include <iostream>
#include <functional>
#include <stdint.h>
#include <cassert>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

// C++11 compat
inline int stoi( std::string const& str ) {
	int i;
	if( (std::istringstream( str ) >> i).fail() ) {
		throw std::invalid_argument( "" );
	}
	return i;
}

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

template<class Func>
struct ScopeExiter {
	ScopeExiter( ScopeExiter<Func>&& lhs ):
		_callback( std::move( lhs._callback ) ),
		_run( lhs._run ) {
		lhs._run = false;
	}

	explicit ScopeExiter( Func const& cb, bool r = true ):
		_callback( cb ),
		_run( r ) {
	}

	~ScopeExiter() {
		if( _run ) {
			try {
				_callback();
			}
			catch( ... ) {}
		}
	}

	private:
		ScopeExiter( ScopeExiter<Func> const& );
		ScopeExiter<Func> const& operator=( ScopeExiter<Func> const& );
		Func const _callback;
		bool _run;
};

template<class Func>
ScopeExiter<Func> scopeExit( Func const& cb ) {
	return ScopeExiter<Func>( cb );
}

namespace tmeta {
	template<class... Tn>
	struct List {
	};

	template<class, class> struct Find;

	template<class... Tn, class T>
	struct Find<List<T, Tn...>, T> {
		static int const value = 0;
	};

	template<class T1, class... Tn, class T>
	struct Find<List<T1, Tn...>, T> {
		static int const value = 1 + Find<List<Tn...>, T>::value;
	};
}

template<class... Tn>
struct Variant {
	int const dTag;

	protected:
		explicit Variant( int t ): dTag( t ) {}
};

template<class, class>
struct VariantImpl;

template<class... Tn, class T>
struct VariantImpl<Variant<Tn...>, T>: Variant<Tn...> {
	static int const sTag = tmeta::Find<tmeta::List<Tn...>, T>::value;

	VariantImpl(): Variant<Tn...>( sTag ) {}
};

template<class T>
struct FalseWrapper {
	FalseWrapper( T const& v ): val( v ) {}

	operator bool() const {
		return false;
	}

	T val;
};

template<class T>
FalseWrapper<T> falseWrap( T const& v ) {
	return FalseWrapper<T>( v );
}

#define VSWITCH( rhs ) \
	if( auto _variant_switch_value_ = falseWrap( rhs ) ) {} else \
	switch( _variant_switch_value_.val->dTag ) {

#define VCASE( type, lhs ) \
		break; \
	} \
	case type::sTag: { \
		type* lhs = static_cast<type*>( _variant_switch_value_.val ); \
		(void)(lhs);

#define VDEFAULT \
		break; \
	} \
	default:

struct IOError: std::exception {
};
