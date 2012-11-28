// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

using namespace std;


template<class T, size_t N>
constexpr size_t size( T const (&)[N] ) {
	return N;
}

constexpr int imod( int a, int b ) {
	return (a * b < 0) ? (a % b + b) : (a % b);
}

template<class T, class... Args>
inline unique_ptr<T> make_unique( Args&&... args ) {
	return unique_ptr<T>( new T( forward<Args>( args )... ) );
}

template<class T>
T readValue( string const& str ) {
	istringstream ifs( str );
	ifs.exceptions( ios_base::failbit | ios_base::badbit );

	T val;
	ifs >> val;
	return val;
}

template<class Func0, class Func1>
void parallel( Func0 const& f0, Func1 const& f1 ) {
	auto slave = async( launch::async, f0 );
	f1();
	slave.get();
}

template<class Func>
struct ScopeExiter {
	ScopeExiter( ScopeExiter<Func> const& ) = delete;
	ScopeExiter<Func>& operator=( ScopeExiter<Func> const& ) = delete;

	ScopeExiter(): _run( false ) {}

	ScopeExiter( ScopeExiter<Func>&& lhs ):
		_callback( move( lhs._callback ) ),
		_run( lhs._run ) {
		lhs._run = false;
	}

	template<class T>
	explicit ScopeExiter( T&& cb, bool r = true ):
		_callback( forward<T>( cb ) ),
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
		Func const _callback;
		bool _run;
};

template<class T>
inline ScopeExiter<typename remove_reference<T>::type> scopeExit( T&& cb ) {
	return ScopeExiter<typename remove_reference<T>::type>( forward<T>( cb ) );
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
	virtual ~Variant() {}
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
	operator bool() const {
		return false;
	}

	T val;
};

template<class T>
inline FalseWrapper<T> falseWrap( T v ) {
	static_assert( is_pointer<T>::value, "" );
	return FalseWrapper<T>{ v };
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
