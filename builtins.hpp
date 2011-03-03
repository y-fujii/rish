// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include "config.hpp"
#include <deque>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <sys/ioctl.h>
//#include <tr1/regrex>
#include "misc.hpp"
#include "glob.hpp"

using namespace std;

namespace builtins {


int chdir( deque<string> const& args, int, int ) {
	if( args.size() != 1 ) {
		return 1;
	}
	//return chdir( args[0].c_str() ) < 0 ? 1 : 0;
	return 0;
}

int showList( deque<string> const& args, int ifd, int ofd ) {
	struct winsize ws;
	if( ioctl( ofd, TIOCGWINSZ, &ws ) < 0 ) {
		return 1;
	}

	int sum1 = 0;
	int sum2 = 0;
	for( deque<string>::const_iterator it = args.begin(); it != args.end(); ++it ) {
		sum1 += it->size();
		sum2 += it->size() * it->size();
	}
	double avg = double( sum1 ) / args.size();
	double var = sqrt( double( sum2 ) / args.size() - avg * avg );
	int ncol = int( ws.ws_col / max( 4.0, avg + var * 2.0 + 2.0) );
	int col = ws.ws_col / ncol;
	assert( col >= 4 );
	int nrow = (args.size() + ncol - 1) / ncol;

	ostringstream buf;
	for( int y = 0; y < nrow; ++y ) {
		for( int x = 0; x < ncol; ++x ) {
			if( y * ncol + x < args.size() ) {
				string str = args[y * ncol + x];
				if( str.size() >= col ) {
					str = str.substr( 0, col - 4 ) + "...";
				}
				buf << setw( col ) << left << str;
			}
		}
		buf << '\n';
	}

	return write( ofd, buf.str().data(), buf.str().size() ) < 0 ? 1 : 0;
}

int strSize( deque<string> const& args, int ifd, int ofd ) {
	ostringstream buf;
	for( int i = 1; i < args.size(); ++i ) {
		buf << args[i].size() << '\n';
	}
	if( write( ofd, buf.str().data(), buf.str().size() ) < 0 ) {
		return 1;
	}

	return 0;
}

/*
int strFind( deque<string> const& args, int ifd, int ofd ) {
	smatch match;
	ostringstream buf;
	for( int i = 2; i < args.size(); ++i ) {
		regex_search( args[i], match, args[1] );
	}
	if( write( ofd, buf.str().data(), buf.str().size() ) < 0 ) {
		return 1;
	}

	return 0;
}

int strMatch( deque<string> const& args, int ifd, int ofd ) {
	if( args.size() != 2 ) {
		return 1;
	}

	ostringstream buf;
	if( regex_match( args[1], args[2] ) ) {
		return 0;
	}
	else {
		return 1;
	}
}

int strReplace( deque<string> const& args, int ifd, int ofd ) {
}
*/

template<class Map>
void register_( Map& map ) {
	map["std.cd"] = chdir;
	map["std.show-list"] = showList;
	map["str.size"] = strSize;
	/*
	map["str.sub"] = strSub;
	map["str.find"] = strFind;
	map["str.match"] = strMatch;
	map["str.replace"] = strReplace;
	map["list.size"] = 
	map["list.sub"] = 
	*/
}


}
