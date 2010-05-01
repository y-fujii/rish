#pragma once

#include <algorithm>
#include <string>
#include <dirent.h>
#include "misc.hpp"


inline uint16_t metaChar( char c ) {
	return c | 0xff00;
}

inline bool isMeta( uint16_t m ) {
	return (m & 0xff00) != 0;
}

inline bool includeMetaChar( MetaString const& str ) {
	for( MetaString::const_iterator it = str.begin(); it != str.end(); ++it ) {
		if( isMeta( *it ) ) {
			return true;
		}
	}
	return false;
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

template<class DstIter, class PatIter>
DstIter expandGlobInner( DstIter dst, PatIter patBgn, PatIter patEnd, std::string const& dirName ) {
	using namespace std;

	PatIter patSlash = std::find( patBgn, patEnd, '/' );

	DIR* dir = opendir( dirName.c_str() );
	dirent entry;
	dirent* result;
	while( true ) {
		if( readdir_r( dir, &entry, &result ) != 0 ) throw exception();
		if( result == NULL ) break;

		if( strcmp( entry.d_name, "." ) == 0 && string( patBgn, patSlash ) == "." ) {
			continue;
		}
		if( strcmp( entry.d_name, ".." ) == 0 && string( patBgn, patSlash ) == ".." ) {
			continue;
		}

		if( patSlash == patEnd ) {
			if( matchGlob( patBgn, patEnd, entry.d_name, entry.d_name + strlen( entry.d_name ) ) ) {
				*dst++ = dirName + entry.d_name;
			}
		}
		else {
			if(
				entry.d_type == DT_DIR &&
				matchGlob( patBgn, patSlash, entry.d_name, entry.d_name + strlen( entry.d_name ) )
			) {
				dst = expandGlobInner( dst, patSlash + 1, patEnd, dirName + entry.d_name + '/' );
			}
		}
	}

	return dst;
}

template<class DstIter>
DstIter expandGlob( DstIter dst, MetaString const& pat ) {
	using namespace std;

	if( !includeMetaChar( pat ) ) {
		*dst++ = string( pat.begin(), pat.end() );
		return dst;
	}

	if( pat.find( '/' ) == 0 ) {
		expandGlobInner( dst, pat.begin() + 1, pat.end(), "/" );
	}
	else {
		expandGlobInner( dst, pat.begin(), pat.end(), "./" );
	}

	return dst;
}
