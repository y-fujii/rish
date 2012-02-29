// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <deque>
#include <string>
#include <stdint.h>
#include <dirent.h>
#include <sys/param.h>
#include "misc.hpp"

using namespace std;


struct MetaString: basic_string<uint16_t> {
	MetaString()                                  : basic_string<uint16_t>() {}
	MetaString( MetaString const& s )             : basic_string<uint16_t>( s ) {}
	MetaString( basic_string<uint16_t> const& s ) : basic_string<uint16_t>( s ) {}
	MetaString( string const& s )                 : basic_string<uint16_t>( s.begin(), s.end() ) {}
	template<class Iter>
	MetaString( Iter bgn, Iter end )              : basic_string<uint16_t>( bgn, end ) {}
};

MetaString::value_type const metaMask = 1 << 15;
MetaString::value_type const star = '*' | metaMask;
MetaString::value_type const any1 = '?' | metaMask;
MetaString::value_type const home = '~' | metaMask;

inline bool isMeta( MetaString::value_type c ) {
	return (c & metaMask) != 0;
}

// O(#ptrn) space, O(#ptrn * #src) time wildcard matcher
inline bool matchGlob( MetaString const& ptrn, string const& src ) {
	deque<bool> mark( ptrn.size() );
	bool prev = true;

	for( size_t i = 0; i < ptrn.size(); ++i ) {
		mark[i] = prev;
		prev &= ptrn[i] == star;
	}

	for( size_t j = 0; j < src.size(); ++j ) {
		prev = false;
		for( size_t i = 0; i < ptrn.size(); ++i ) {
			bool curr;
			if( ptrn[i] == star ) {
				curr = prev | mark[i];
				prev = curr;
			}
			else if( ptrn[i] == any1 || ptrn[i] == src[j] ) {
				curr = prev;
				prev = mark[i];
			}
			else {
				curr = prev;
				prev = false;
			}
			mark[i] = curr;
		}
	}

	return prev;
}

template<class DstIter>
DstIter listDir( string const& root, DstIter dstIt ) {
	char const* r = root.size() == 0 ? "." : root.c_str();
	DIR* dir = opendir( r );
	if( dir == NULL ) {
		throw IOError();
	}
	ScopeExit closer( bind( closedir, dir ) );

	while( true ) {
		dirent entry;
		dirent* result;
		if( readdir_r( dir, &entry, &result ) != 0 ) {
			throw IOError();
		}
		if( result == NULL ) {
			break;
		}
		string name( entry.d_name );
		if( name != "." && name != ".." ) {
			*dstIt++ = make_pair( name, int( entry.d_type ) );
		}
	}

	return dstIt;
}

template<class DstIter>
DstIter expandGlobRec( string const& root, MetaString const& ptrn, DstIter dstIt ) {
	size_t slash = ptrn.find( '/' );
	if( slash == MetaString::npos ) {
		deque<pair<string, int> > dirs;
		listDir( root, back_inserter( dirs ) );
		for( deque<pair<string, int> >::const_iterator it = dirs.begin(); it != dirs.end(); ++it ) {
			if( !(it->second & DT_DIR) && matchGlob( ptrn, it->first ) ) {
				*dstIt++ = root + it->first;
			}
		}
	}
	else {
		assert( slash != 0 );
		MetaString base = ptrn.substr( 0, slash );
		MetaString rest = ptrn.substr( slash + 1 );

		if( base == MetaString( "." ) || base == MetaString( ".." ) ) {
			dstIt = expandGlobRec( root + string( base.begin(), base.end() ) + "/", rest, dstIt );
		}
		else {
			deque<pair<string, int> > dirs;
			listDir( root, back_inserter( dirs ) );
			for( deque<pair<string, int> >::const_iterator it = dirs.begin(); it != dirs.end(); ++it ) {
				if( (it->second & DT_DIR) && matchGlob( base, it->first ) ) {
					if( rest.size() == 0 ) {
						*dstIt++ = root + it->first + "/";
					}
					else {
						dstIt = expandGlobRec( root + it->first + "/", rest, dstIt );
					}
				}
			}
		}
	}

	return dstIt;
}

template<class DstIter>
DstIter expandGlob( MetaString const& src, DstIter dstIt ) {
	MetaString ptrn;
	if( src.size() >= 1 && src[0] == home ) {
		ptrn = MetaString( getenv( "HOME" ) ) + src.substr( 1 );
	}
	else {
		ptrn = src;
	}

	if( !any_of( ptrn.begin(), ptrn.end(), isMeta ) ) {
		*dstIt++ = string( ptrn.begin(), ptrn.end() );
		return dstIt;
	}

	if( ptrn.size() >= 1 && ptrn[0] == '/' ) {
		return expandGlobRec( "/", ptrn.substr( 1 ), dstIt );
	}
	else {
		return expandGlobRec( "", ptrn, dstIt );
	}
}
