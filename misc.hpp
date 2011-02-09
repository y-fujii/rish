#pragma once

#include <typeinfo>
#include <algorithm>
#include <string>
#include <assert.h>
#include <stdint.h>

#if __cplusplus > 199711L
#include <cuchar>
#else
#include <wchar.h>
namespace std {
	typedef uint32_t char32_t;
	typedef basic_string<char32_t> u32string;

	/*
	inline size_t mbrtoc32( char32_t* dst, char const* src, size_t n, mbstate_t* state ) {
		return mbrtowc( dst, src, n, state );
	}

	inline size_t c32rtomb( char* dst, char32_t src, mbstate_t* state ) {
		return wcrtomb( dst, src, state );
	}
	*/

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
		return std::find_if( bgn, end, f ) != end;
	}

	template<class Iter, class Func>
	inline bool all_of( Iter bgn, Iter end, Func f ) {
		return std::find_if( bgn, end, std::not1( f ) ) == end;
	}
}
#endif

#define MATCH( c ) break; case (c):
#define OTHERWISE break; default:
#define RANGE( c ) (c).begin(), (c).end()

template<class Value, class Callee>
struct call_iterator {
	explicit call_iterator( Callee const& c ): _callee( c ) {
	}

	call_iterator<Value, Callee>& operator=( Value& val ) {
		_callee( val );
		return *this;
	}

	call_iterator<Value, Callee>& operator*() {
		return *this;
	}

	call_iterator<Value, Callee>& operator++() {
		return *this;
	}

	call_iterator<Value, Callee> operator++( int ) {
		return *this;
	}

	private:
		Callee const& _callee;
};

template<class Value, class Callee>
call_iterator<Value, Callee> caller( Callee const& c ) {
	return call_iterator<Value, Callee>( c );
}

inline std::string slice( std::string const& str, int x, int y ) {
	if( x < 0 ) {
		x += str.size();
	}
	if( y < 0 ) {
		y += str.size();
	}
	assert( x < y );
	return str.substr( x, y );
}

/*
typedef uintptr_t TypeId;

template<class T>
inline TypeId typeIdFast() {
	return reinterpret_cast<uintptr_t>( &typeid( T ) );
}

template<class T>
inline TypeId typeIdFast( T& t ) {
	return reinterpret_cast<uintptr_t>( &typeid( t ) );
}
*/
