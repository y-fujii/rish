#pragma once

#include "misc.hpp"


inline uint16_t metaChar( char c ) {
	return c | 0xff00;
}

inline bool isMeta( uint16_t m ) {
	return (m & 0xff00) != 0;
}

template<class PatIter, class SrcIter>
bool matchWildcard( PatIter patIt, PatIter patEnd, SrcIter srcIt, SrcIter srcEnd ) {
	if( patIt == patEnd ) {
		return srcIt == srcEnd;
	}

	switch( *patIt ) {
		MATCH( metaChar( '*' ) ) {
			++patIt;
			do {
				if( matchWildcard( patIt, patEnd, srcIt, srcEnd ) ) {
					return true;
				}
			} while( srcIt++ != srcEnd );

			return false;
		}
		MATCH( metaChar( '?' ) ) {
			return (
				srcIt != srcEnd &&
				matchWildcard( ++patIt, patEnd, ++srcIt, srcEnd )
			);
		}
		OTHERWISE {
			return (
				srcIt != srcEnd &&
				*srcIt == *patIt &&
				matchWildcard( ++patIt, patEnd, ++srcIt, srcEnd )
			);
		}
	}
}
