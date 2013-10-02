// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license

#include <cassert>
#include <cmath>
#include <deque>
#include <string>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

using namespace std;


void pprint( ostream& out, deque<string> const& srcs ) {
	winsize ws;
	if( ioctl( 1, TIOCGWINSZ, &ws ) < 0 ) {
		throw;
	}

	int sum1 = 0;
	int sum2 = 0;
	for( auto const& v: srcs ) {
		sum1 += v.size();
		sum2 += v.size() * v.size();
	}
	double avg = double( sum1 ) / srcs.size();
	double var = sqrt( double( sum2 ) / srcs.size() - avg * avg );
	size_t ncol = ws.ws_col / (avg + var * 2.0 + 2.0);
	size_t nrow = (srcs.size() + ncol - 1) / ncol;
	size_t col = ws.ws_col / ncol;
	assert( col >= 2 );

	for( size_t y = 0; y < nrow; ++y ) {
		for( size_t x = 0; x < ncol; ++x ) {
			if( x * nrow + y < srcs.size() ) {
				string const& e = srcs[x * nrow + y];
				if( e.size() >= col ) {
					out << e.substr( 0, col - 2 ) << "$ ";
				}
				else {
					out << setw( col ) << left << e;
				}
			}
		}
		out << '\n';
	}
}

int main( int, char** ) {
	try {
		deque<string> srcs;
		while( true ) {
			string buf;
			if( !getline( cin, buf ) ) {
				break;
			}
			srcs.push_back( move( buf ) );
		}

		pprint( cout, srcs );
	}
	catch( ... ) {
		return 1;
	}

	return 0;
}
