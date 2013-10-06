// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <system_error>
#include <dirent.h>
#include <sys/param.h>
#include "misc.hpp"

using namespace std;


struct MetaString: basic_string<uint16_t> {
	MetaString()                                  = default;
	MetaString( MetaString&& )                    = default;
	MetaString( MetaString const& )               = default;
	MetaString( basic_string<uint16_t>&& s )      : basic_string<uint16_t>( move( s ) ) {}
	MetaString( basic_string<uint16_t> const& s ) : basic_string<uint16_t>( s ) {}
	MetaString( string const& s )                 : basic_string<uint16_t>( s.begin(), s.end() ) {}
	template<class Iter>
	MetaString( Iter bgn, Iter end )              : basic_string<uint16_t>( bgn, end ) {}

	explicit operator string() const {
		return string( cbegin(), cend() );
	}
};

inline bool operator!=( MetaString const& lhs, string rhs ) {
	return lhs != MetaString( rhs );
}

uint16_t const metaMask = 1 << 15;
uint16_t const star = '*' | metaMask;
uint16_t const any1 = '?' | metaMask;
uint16_t const home = '~' | metaMask;

inline bool isMeta( uint16_t c ) {
	return (c & metaMask) != 0;
}

// O(#ptrn) space, O(#ptrn * #src) time wildcard matcher
inline bool matchGlob( basic_string<uint16_t> const& ptrn, string const& src ) {
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
	assert( root.size() != 0 );
	DIR* dir = opendir( root.c_str() );
	if( dir == nullptr ) {
		throw system_error( errno, system_category() );
	}
	auto closer = scopeExit( bind( closedir, dir ) );

	while( true ) {
		// readdir() IS reentrant.
		dirent* entry = readdir( dir );
		if( entry == nullptr ) {
			break;
		}
		string name( entry->d_name );
		if( name != "." && name != ".." ) {
			*dstIt++ = make_tuple( name, int( entry->d_type ) );
		}
	}

	return dstIt;
}

template<class DstIter>
DstIter expandGlobRec( string const& root, basic_string<uint16_t> const& ptrn, DstIter dstIt ) {
	size_t slash = ptrn.find( '/' );
	if( slash == basic_string<uint16_t>::npos ) {
		deque<tuple<string, int>> dirs;
		try {
			listDir( root, back_inserter( dirs ) );
		}
		catch( system_error const& ) {}

		for( auto const& dir: dirs ) {
			if( !(get<1>( dir ) & DT_DIR) && matchGlob( ptrn, get<0>( dir ) ) ) {
				*dstIt++ = root + get<0>( dir );
			}
		}
	}
	else {
		assert( slash != 0 );
		auto base = ptrn.substr( 0, slash );
		auto rest = ptrn.substr( slash + 1 );

		if( base == basic_string<uint16_t>{ '.' } ||
		    base == basic_string<uint16_t>{ '.', '.' } ) {
			dstIt = expandGlobRec( root + string( base.begin(), base.end() ) + "/", rest, dstIt );
		}
		else {
			deque<tuple<string, int>> dirs;
			try {
				listDir( root, back_inserter( dirs ) );
			}
			catch( system_error const& ) {}

			for( auto const& dir: dirs ) {
				if( (get<1>( dir ) & DT_DIR) && matchGlob( base, get<0>( dir ) ) ) {
					if( rest.size() == 0 ) {
						*dstIt++ = root + get<0>( dir ) + "/";
					}
					else {
						dstIt = expandGlobRec( root + get<0>( dir ) + "/", rest, dstIt );
					}
				}
			}
		}
	}

	return dstIt;
}

template<class DstIter>
DstIter expandGlob( MetaString const& src, string const& cwd, DstIter dstIt ) {
	if( !any_of( src.begin(), src.end(), isMeta ) ) {
		*dstIt++ = string( src );
		return dstIt;
	}

	assert( src.size() > 0 );
	if( src[0] == '/' ) {
		return expandGlobRec( "/", src.substr( 1 ), dstIt );
	}
	else {
		return expandGlobRec( cwd + "/", src, dstIt );
	}
}
