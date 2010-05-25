#pragma once

#include <algorithm>
#include <string>
#include <dirent.h>
#include "misc.hpp"


inline uint16_t metaChar( char c ) {
	return c | 0xff00;
}

inline bool isMetaChar( uint16_t m ) {
	return (m & 0xff00) != 0;
}

template<class PatIter, class SrcIter>
bool matchGlob( PatIter ptnIt, PatIter ptnEnd, SrcIter srcIt, SrcIter srcEnd ) {
	if( ptnIt == ptnEnd ) {
		return srcIt == srcEnd;
	}

	switch( *ptnIt ) {
		MATCH( '*' | 0xff00 ) {
			++ptnIt;
			do {
				if( matchGlob( ptnIt, ptnEnd, srcIt, srcEnd ) ) {
					return true;
				}
			} while( srcIt++ != srcEnd );

			return false;
		}
		MATCH( '?' | 0xff00 ) {
			return (
				srcIt != srcEnd &&
				matchGlob( ++ptnIt, ptnEnd, ++srcIt, srcEnd )
			);
		}
		OTHERWISE {
			return (
				srcIt != srcEnd &&
				*srcIt == *ptnIt &&
				matchGlob( ++ptnIt, ptnEnd, ++srcIt, srcEnd )
			);
		}
	}
}

template<class DstIter, class PatIter>
DstIter expandGlobInner( DstIter dst, PatIter ptnBgn, PatIter ptnEnd, std::string const& dirName ) {
	using namespace std;

	PatIter ptnSlash = find( ptnBgn, ptnEnd, '/' );

	DIR* dir = opendir( dirName.c_str() );
	dirent entry;
	dirent* result;
	while( true ) {
		if( readdir_r( dir, &entry, &result ) != 0 ) throw exception();
		if( result == NULL ) break;

		if( strcmp( entry.d_name, "." ) == 0 && string( ptnBgn, ptnSlash ) == "." ) {
			continue;
		}
		if( strcmp( entry.d_name, ".." ) == 0 && string( ptnBgn, ptnSlash ) == ".." ) {
			continue;
		}

		if( ptnSlash == ptnEnd ) {
			if( matchGlob( ptnBgn, ptnEnd, entry.d_name, entry.d_name + strlen( entry.d_name ) ) ) {
				*dst++ = dirName + entry.d_name;
			}
		}
		else {
			if(
				entry.d_type == DT_DIR &&
				matchGlob( ptnBgn, ptnSlash, entry.d_name, entry.d_name + strlen( entry.d_name ) )
			) {
				dst = expandGlobInner( dst, ptnSlash + 1, ptnEnd, dirName + entry.d_name + '/' );
			}
		}
	}

	return dst;
}

template<class DstIter>
DstIter expandGlob( DstIter dst, MetaString const& ptn ) {
	using namespace std;

	if( !any( ptn.begin(), ptn.end(), isMetaChar ) ) {
		*dst++ = string( ptn.begin(), ptn.end() );
		return dst;
	}

	if( ptn.size() >= 1 && ptn[0] == '/' ) {
		expandGlobInner( dst, ptn.begin() + 1, ptn.end(), "/" );
	}
	else {
		expandGlobInner( dst, ptn.begin(), ptn.end(), "./" );
	}

	return dst;
}
