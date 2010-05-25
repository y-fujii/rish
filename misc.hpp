#pragma once

#include <typeinfo>
#include <algorithm>
#include <string>
#include <stdint.h>


#define MATCH( c ) break; case (c):
#define OTHERWISE break; default:

typedef std::basic_string<uint16_t> MetaString;
typedef uintptr_t TypeId;

// The codes below are NOT LEGAL but work correctly on most compilers.

template<class T>
inline TypeId typeIdFast() {
	return reinterpret_cast<uintptr_t>( &typeid( T ) );
}

template<class T>
inline TypeId typeIdFast( T& t ) {
	return reinterpret_cast<uintptr_t>( &typeid( t ) );
}

template<class Iter, class Func>
inline bool any( Iter bgn, Iter end, Func f ) {
	return std::find_if( bgn, end, f ) != end;
}

template<class Iter, class Func>
inline bool all( Iter bgn, Iter end, Func f ) {
	return std::find_if( bgn, end, std::not1( f ) ) == end;
}
