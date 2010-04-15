#pragma once
// The codes below are NOT LEGAL but work correctly on most compilers.

#include <stdint.h>


typedef uintptr_t TypeId;

template<class T>
inline TypeId typeIdFast() {
	return reinterpret_cast<uintptr_t>( &typeid( T ) );
}

template<class T>
inline TypeId typeIdFast( T& t ) {
	return reinterpret_cast<uintptr_t>( &typeid( t ) );
}
