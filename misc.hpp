#pragma once

#include <typeinfo>
#include <stdint.h>


#define MATCH( c ) break; case (c):
#define OTHERWISE break; default:

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
