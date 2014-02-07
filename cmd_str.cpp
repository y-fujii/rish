// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license

#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include "misc.hpp"

using namespace std;


int error() {
	cerr << "error.\n";
	return -1;
}

int main( int argc, char** argv ) {
	int opt;
	while( opt = getopt( argc, argv, "" ), opt != -1 ) {
		return error();
	}
	if( optind >= argc ) {
		return error();
	}

	string cmd = argv[optind++];
	size_t n = argc - optind;

	if( cmd == "split" ) {
		char c;
		while( cin.read( &c, 1 ) ) {
			cout << c << '\n';
		}
	}
	else if( cmd == "join" ) {
		string buf;
		while( getline( cin, buf ) ) {
			cout << buf;
		}
		cout << '\n';
	}
	else if( cmd == "cmp" ) {
		if( n != 1 ) {
			throw exception();
		}
		string src( argv[optind] );

		string buf;
		while( getline( cin, buf ) ) {
			int t = (
				buf < src ? -1 :
				buf > src ? +1 :
				             0
			);
			cout << t << '\n';
		}
	}
	else if( cmd == "len" ) {
		string buf;
		while( getline( cin, buf ) ) {
			cout << buf.size() << '\n';
		}
	}
	else if( cmd == "index" ) {
		vector<int64_t> idx( n );
		for( size_t i = 0; i < n; ++i ) {
			idx[i] = strtoll( argv[optind + i], nullptr, 10 );
		}

		string buf;
		while( getline( cin, buf ) ) {
			for( int64_t i: idx ) {
				if( buf.size() == 0 ) {
					return error();
				}
				int64_t j = imod( i, buf.size() );
				cout << buf[j] << '\n';
			}
		}
	}
	else if( cmd == "slice" ) {
		if( n != 2 ) {
			throw exception();
		}
		int64_t bgn = strtoll( argv[optind + 0], nullptr, 10 );
		int64_t end = strtoll( argv[optind + 1], nullptr, 10 );

		string buf;
		while( getline( cin, buf ) ) {
			int64_t b = imod( bgn, buf.size() );
			int64_t e = imod( end, buf.size() );
			if( b > e ) {
				return error();
			}
			cout << buf.substr( b, e - b ) << '\n';
		}
	}
	else if( cmd == "match" ) {
		assert( false );
	}
	else if( cmd == "subst" ) {
		assert( false );
	}
	else {
		return error();
	}

	return 0;
}
