// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <deque>
#include <cassert>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
//#include <tr1/regrex>
#include "misc.hpp"
#include "unix.hpp"
#include "glob.hpp"

using namespace std;

namespace builtins {


int changeDir( deque<string> const& args, int, int ) {
	if( args.size() != 1 ) {
		return 1;
	}
	checkSysCall( chdir( args[0].c_str() ) );
	return 0;
}

int setEnv( deque<string> const& args, int, int ) {
	if( args.size() != 2 ) {
		return 1;
	}
	checkSysCall( setenv( args[0].c_str(), args[1].c_str(), 1 ) );
	return 0;
}

int getEnv( deque<string> const& args, int, int ofd ) {
	if( args.size() != 1 ) {
		return 1;
	}
	char* val = getenv( args[0].c_str() );
	if( val == NULL ) {
		return 1;
	}
	writeAll( ofd, string( val ) + '\n' );
	return 0;
}

int wait( deque<string> const& args, int, int ) {
	if( args.size() != 1 ) {
		return 1;
	}
	pid_t pid;
	if( (istringstream( args[0] ) >> pid).fail() ) {
		return 1;
	}
	int status;
	checkSysCall( waitpid( pid, &status, 0 ) );
	return WEXITSTATUS( status );
}

int showList( deque<string> const& args, int, int ofd ) {
	struct winsize ws;
	checkSysCall( ioctl( ofd, TIOCGWINSZ, &ws ) );

	int sum1 = 0;
	int sum2 = 0;
	for( deque<string>::const_iterator it = args.begin(); it != args.end(); ++it ) {
		sum1 += it->size();
		sum2 += it->size() * it->size();
	}
	double avg = double( sum1 ) / args.size();
	double var = sqrt( double( sum2 ) / args.size() - avg * avg );
	size_t ncol = ws.ws_col / (avg + var * 2.0 + 2.0);
	size_t nrow = (args.size() + ncol - 1) / ncol;
	size_t col = ws.ws_col / ncol;
	assert( col >= 2 );

	ostringstream buf;
	for( size_t y = 0; y < nrow; ++y ) {
		for( size_t x = 0; x < ncol; ++x ) {
			if( x * nrow + y < args.size() ) {
				string const& e = args[x * nrow + y];
				if( e.size() >= col ) {
					buf << e.substr( 0, col - 2 ) << "$ ";
				}
				else {
					buf << setw( col ) << left << e;
				}
			}
		}
		buf << '\n';
	}

	writeAll( ofd, buf.str() );
	return 0;
}

int strSize( deque<string> const& args, int, int ofd ) {
	ostringstream buf;
	for( deque<string>::const_iterator it = args.begin(); it != args.end(); ++it ) {
		buf << it->size() << '\n';
	}

	writeAll( ofd, buf.str() );
	return 0;
}

int listSize( deque<string> const& args, int, int ofd ) {
	ostringstream buf;
	buf << args.size() << '\n';
	writeAll( ofd, buf.str() );
	return 0;
}

/*
int strFind( deque<string> const& args, int ifd, int ofd ) {
	smatch match;
	ostringstream buf;
	for( int i = 2; i < args.size(); ++i ) {
		regex_search( args[i], match, args[1] );
	}

	writeAll( ofd, buf.str() );
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
	map["std.cd"] = changeDir;
	map["std.show-list"] = showList;
	map["sys.setenv"] = setEnv;
	map["sys.getenv"] = getEnv;
	map["sys.wait"] = wait;
	map["str.size"] = strSize;
	/*
	map["str.sub"] = strSub;
	map["str.find"] = strFind;
	map["str.match"] = strMatch;
	map["str.replace"] = strReplace;
	map["list.sub"] = 
	*/
	map["list.size"] = listSize;
}


}
