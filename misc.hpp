#pragma once

#include <algorithm>
#include <string>
#include <cassert>
#include <stdint.h>

namespace std {
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
