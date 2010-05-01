#pragma once

#include "misc.hpp"


inline uint16_t metaChar( char c ) {
	return c | 0xff00;
}

inline bool isMeta( uint16_t m ) {
	return (m & 0xff00) != 0;
}

template<class PatIter, class SrcIter>
bool matchGlob( PatIter patIt, PatIter patEnd, SrcIter srcIt, SrcIter srcEnd ) {
	if( patIt == patEnd ) {
		return srcIt == srcEnd;
	}

	switch( *patIt ) {
		MATCH( '*' | 0xff00 ) {
			++patIt;
			do {
				if( matchGlob( patIt, patEnd, srcIt, srcEnd ) ) {
					return true;
				}
			} while( srcIt++ != srcEnd );

			return false;
		}
		MATCH( '?' | 0xff00 ) {
			return (
				srcIt != srcEnd &&
				matchGlob( ++patIt, patEnd, ++srcIt, srcEnd )
			);
		}
		OTHERWISE {
			return (
				srcIt != srcEnd &&
				*srcIt == *patIt &&
				matchGlob( ++patIt, patEnd, ++srcIt, srcEnd )
			);
		}
	}
}

template<class DstIter, class SrcIter>
void expandGlob( DstIter dst, SrcIter bgn, SrcIter end ) {
}
