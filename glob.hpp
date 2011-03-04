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

// O(#ptn) space, O(#ptn * #src) time wildcard matcher
inline bool matchGlob( MetaString const& ptn, string const& src ) {
	deque<bool> mark( ptn.size() );
	bool prev = true;

	for( size_t i = 0; i < ptn.size(); ++i ) {
		mark[i] = prev;
		prev &= ptn[i] == star;
	}

	for( size_t j = 0; j < src.size(); ++j ) {
		prev = false;
		for( size_t i = 0; i < ptn.size(); ++i ) {
			bool curr;
			if( ptn[i] == star ) {
				curr = prev | mark[i];
				prev = curr;
			}
			else if( ptn[i] == any1 || ptn[i] == src[j] ) {
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
	DIR* dir = opendir( root.c_str() );
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
			*dstIt++ = make_pair<string, int>( name, entry.d_type );
		}
	}

	return dstIt;
}

template<class DstIter>
DstIter expandGlobRec( string const& root, MetaString const& ptn, DstIter dstIt ) {
	size_t slash = ptn.find( '/' );
	if( slash == MetaString::npos ) {
		deque<pair<string, int> > dirs;
		listDir( root, back_inserter( dirs ) );
		for( deque<pair<string, int> >::const_iterator it = dirs.begin(); it != dirs.end(); ++it ) {
			if( !(it->second & DT_DIR) && matchGlob( ptn, it->first ) ) {
				*dstIt++ = root + it->first;
			}
		}
	}
	else {
		assert( slash != 0 );
		MetaString base = ptn.substr( 0, slash );
		MetaString rest = ptn.substr( slash + 1 );

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
	if( !any_of( src.begin(), src.end(), isMeta ) ) {
		*dstIt++ = string( src.begin(), src.end() );
		return dstIt;
	}
	if( src.size() >= 2 && src[0] == home && src[1] == '/' ) {
		return expandGlobRec( getenv( "HOME" ), src.substr( 2 ), dstIt );
	}
	else if( src.size() >= 1 && src[0] == '/' ) {
		return expandGlobRec( "/", src.substr( 1 ), dstIt );
	}
	else {
		return expandGlobRec( "./", src, dstIt );
	}
}
