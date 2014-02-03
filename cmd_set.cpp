// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license

#include <algorithm>
#include <set>
#include <string>
#include <iostream>
#include <unistd.h>

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

	string cmd( argv[optind++] );
	set<string> ys( argv + optind, argv + argc );

	if( cmd == "isect" ) {
		string buf;
		while( cin >> buf ) {
			if( ys.find( buf ) != ys.end() ) {
				cout << buf << '\n';
			}
		}
	}
	else if( cmd == "union" ) {
		string buf;
		while( cin >> buf ) {
			auto it = ys.find( buf );
			if( it != ys.end() ) {
				ys.erase( it );
			}
			cout << buf << '\n';
		}
		for( string const& y: ys ) {
			cout << y << '\n';
		}
	}
	else if( cmd == "diff" ) {
		string buf;
		while( cin >> buf ) {
			if( ys.find( buf ) == ys.end() ) {
				cout << buf << '\n';
			}
		}
	}
	else if( cmd == "contain" ) {
		set<string> xs;
		string buf;
		while( cin >> buf ) {
			xs.insert( buf );
		}
		bool r = all_of( ys.begin(), ys.end(),
			[&xs]( string const& y ) {
				return xs.find( y ) != xs.end();
			}
		);

		return r ? 0 : 1;
	}
	else {
		return error();
	}

	return 0;
}
