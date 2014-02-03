// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license

#include <algorithm>
#include <vector>
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

	string cmd = argv[optind++];
	char** ysBgn = argv + optind;
	char** ysEnd = argv + argc;
	sort( ysBgn, ysEnd );
	ysEnd = unique( ysBgn, ysEnd );

	if( cmd == "isect" ) {
		string buf;
		while( cin >> buf ) {
			if( binary_search( ysBgn, ysEnd, buf ) ) {
				cout << buf << '\n';
			}
		}
	}
	else if( cmd == "union" ) {
		vector<bool> flags( ysEnd - ysBgn, true );
		string buf;
		while( cin >> buf ) {
			char** it = lower_bound( ysBgn, ysEnd, buf );
			if( it != ysEnd ) {
				flags[it - ysBgn] = false;
			}
			cout << buf << '\n';
		}
		for( int i = 0; i < ysEnd - ysBgn; ++i ) {
			if( flags[i] ) {
				cout << ysBgn[i] << '\n';
			}
		}
	}
	else if( cmd == "diff" ) {
		string buf;
		while( cin >> buf ) {
			if( !binary_search( ysBgn, ysEnd, buf ) ) {
				cout << buf << '\n';
			}
		}
	}
	else if( cmd == "contain" ) {
		vector<string> xs;
		string buf;
		while( cin >> buf ) {
			xs.push_back( buf );
		}
		sort( xs.begin(), xs.end() );

		bool r = includes( xs.begin(), xs.end(), ysBgn, ysEnd );
		return r ? 0 : 1;
	}
	else {
		return error();
	}

	return 0;
}
