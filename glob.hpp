#pragma once

#include <deque>
#include <string>
#include <stdint.h>
#include <dirent.h>
#include "misc.hpp"
#include "exception.hpp"

using namespace std;


struct MetaString: basic_string<uint16_t> {
	MetaString()                                  : basic_string() {}
	MetaString( MetaString const& s )             : basic_string( s ) {}
	MetaString( basic_string<uint16_t> const& s ) : basic_string( s ) {}
	MetaString( string const& s )                 : basic_string( s.begin(), s.end() ) {}
	template<class Iter>
	MetaString( Iter bgn, Iter end )              : basic_string( bgn, end ) {}
};

MetaString::value_type const metaMask = 1 << 15;
MetaString::value_type const star = '*' | metaMask;
MetaString::value_type const any1 = '?' | metaMask;
MetaString::value_type const home = '~' | metaMask;

template<class String>
String pathJoin( String const& x, String const& y ) {
	if( y.size() > 0 && y[0] == '/' ) {
		return y;
	}
	else {
		if( x.size() > 0 && x[x.size() - 1] == '/' ) {
			return x + y;
		}
		else {
			return x + "/" + y;
		}
	}
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
	try {
		while( true ) {
			dirent entry;
			dirent* result;
			if( readdir_r( dir, &entry, &result ) != 0 ) {
				throw IOError();
			}
			if( result == NULL ) {
				break;
			}
			*dstIt++ = make_pair( entry.d_type, entry.d_name );
		}
	}
	catch( ... ) {
		closedir( dir );
		throw;
	}
	if( closedir( dir ) < 0 ) {
		throw IOError();
	}

	return dstIt;
}

template<class DstIter>
DstIter walkDir( string const& root, DstIter dstIt ) {
	deque< pair<int, string> > tmp;
	listDir( root, back_inserter( tmp ) );

	for( deque< pair<int, string> >::const_iterator it = tmp.begin(); it != tmp.end(); ++it ) {
		if( it->second == "." || it->second == ".." ) {
			continue;
		}
		if( it->first & DT_DIR ) {
			string name = root + it->second + "/";
			*dstIt++ = make_pair( it->first, name );
			dstIt = walkDir( name, dstIt );
		}
		else {
			string name = root + it->second;
			*dstIt++ = make_pair( it->first, name );
		}
	}

	return dstIt;
}

/*
template<class DstIter>
DstIter expandGlob( string const& root, MetaString const& pattern, DstIter dst ) {
	size_t slash = pattern.find( '/' );
	if( slash == MetaString::npos ) {
		deque< pair<int, string> > dirs;
		listDir( root, back_inserter( dirs ) );
		for( deque< pair<int, string> >::const_iterator it = dirs.begin(); it != dirs.end(); ++it ) {
			if( it->first & DT_DIR ) {
				continue;
			}
			if( matchGlob( pattern, it->second.begin(), it->second.end() ) ) {
				*dst++ = root + it->second;
			}
		}
	}
	else {
		MetaString base = pattern.substr( 0, slash );
		MetaString rest = pattern.substr( slash + 1 );

		if( base == "." || base == ".." ) {
			dst = expandGlob( root + string( base.begin(), base.end() ) + "/", rest, dst );
		}
		else {
			deque< pair<int, string> > dirs;
			string root2;
			if( base == "" ) {
				root2 = "/";
			} else {
				root2 = root;
			}
			listDir( root2, back_inserter( dirs ) );
			for( deque< pair<int, string> >::const_iterator it = dirs.begin(); it != dirs.end(); ++it ) {
				if( !(it->first & DT_DIR) || it->second == "." || it->second == ".." ) {
					continue;
				}
				if( matchGlob( base, it->second.begin(), it->second.end() ) ) {
					if( rest != "" ) {
						dst = expandGlob( root2 + it->second + "/", rest, dst );
					}
					else {
						*dst++ = root2 + it->second;
					}
				}
			}
		}
	}

	return dst;
}
*/

template<class DstIter>
DstIter expandGlob( MetaString const& src, DstIter dst ) {
	if( src.size() > 0 && src[0] == home ) {
		*dst++ = getenv( "HOME" ) + string( src.begin() + 1, src.end() );
	}
	else {
		*dst++ = string( src.begin(), src.end() );
	}
	return dst;
}
