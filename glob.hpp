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

// O(#pattern) space, O(#pattern * #src) time wildcard matcher
template<class SrcIter>
bool matchGlob( MetaString const& pattern, SrcIter srcBgn, SrcIter srcEnd ) {
	deque<bool> mark( pattern.size() );
	bool prev = true;

	for( size_t i = 0; i < pattern.size(); ++i ) {
		mark[i] = prev;
		prev &= pattern[i] == star;
	}

	for( SrcIter srcIt = srcBgn; srcIt != srcEnd; ++srcIt ) {
		prev = false;
		for( size_t i = 0; i < pattern.size(); ++i ) {
			bool curr;
			if( pattern[i] == star ) {
				curr = prev | mark[i];
				prev = curr;
			}
			else if( pattern[i] == any1 || pattern[i] == *srcIt ) {
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
		if( string( entry.d_name ) != "." && string( entry.d_name ) != ".." ) {
			*dstIt++ = entry;
		}
	}

	return dstIt;
}

/*
template<class DstIter>
DstIter walkDir( string const& root, DstIter dstIt ) {
	deque< pair<int, string> > tmp;
	listDir( root, back_inserter( tmp ) );

	for( deque<dirent>::const_iterator it = tmp.begin(); it != tmp.end(); ++it ) {
		if( it->d_type & DT_DIR ) {
			string name = root + it->d_name + "/";
			*dstIt++ = make_pair( it->d_name, name );
			dstIt = walkDir( name, dstIt );
		}
		else {
			string name = root + it->second;
			*dstIt++ = make_pair( it->first, name );
		}
	}

	return dstIt;
}
*/

template<class DstIter>
DstIter expandGlobRec( string const& root, MetaString const& pattern, DstIter dst ) {
	size_t slash = pattern.find( '/' );
	if( slash == MetaString::npos ) {
		deque<dirent> dirs;
		listDir( root, back_inserter( dirs ) );
		for( deque<dirent>::const_iterator it = dirs.begin(); it != dirs.end(); ++it ) {
			if(
				!(it->d_type & DT_DIR) &&
				matchGlob( pattern, it->d_name, it->d_name + it->d_namlen )
			) {
				*dst++ = root + it->d_name;
			}
		}
	}
	else {
		MetaString base = pattern.substr( 0, slash );
		MetaString rest = pattern.substr( slash + 1 );

		/*
		if( base == MetaString( "." ) || base == MetaString( ".." ) ) {
			dst = expandGlobRec( root + string( base.begin(), base.end() ) + "/", rest, dst );
		}
		*/
		if( all_of( base.begin(), base.end(), isMeta ) ) {
			dst = expandGlobRec( root + string( base.begin(), base.end() ) + "/", rest, dst );
		}
		else {
			assert( slash != 0 );
			deque<dirent> dirs;
			listDir( root, back_inserter( dirs ) );
			for( deque<dirent>::const_iterator it = dirs.begin(); it != dirs.end(); ++it ) {
				if(
					(it->d_type & DT_DIR) &&
					matchGlob( base, it->d_name, it->d_name + it->d_namlen )
				) {
					if( rest == MetaString( "" ) ) {
						*dst++ = root + it->d_name;
					}
					else {
						dst = expandGlobRec( root + it->d_name + "/", rest, dst );
					}
				}
			}
		}
	}

	return dst;
}

#include <iostream>
template<class DstIter>
DstIter expandGlob( MetaString const& src, DstIter dstIt ) {
	if( src.size() >= 2 && src[0] == home && src[1] == '/' ) {
		return expandGlobRec( getenv( "HOME" ), src.substr( 2 ), dstIt );
	}
	else if( src.size() >= 1 && src[0] == '/' ) {
		return expandGlobRec( "/", src.substr( 1 ), dstIt );
	}
	else {
		char buf[MAXPATHLEN];
		getcwd( buf, MAXPATHLEN );
		return expandGlobRec( string( buf ) + "/", src, dstIt );
	}
}
