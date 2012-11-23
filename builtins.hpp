// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <cassert>
#include <cmath>
#include <deque>
#include <iomanip>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
//#include <tr1/regrex>
#include "misc.hpp"
#include "glob.hpp"
#include "eval.hpp"
#include "unix.hpp"

using namespace std;

namespace builtins {


int changeDir( deque<string> const& args, Evaluator&, int, int ) {
	if( args.size() != 1 ) {
		return 1;
	}
	checkSysCall( chdir( args[0].c_str() ) );
	return 0;
}

int setEnv( deque<string> const& args, Evaluator&, int, int ) {
	if( args.size() != 2 ) {
		return 1;
	}
	checkSysCall( setenv( args[0].c_str(), args[1].c_str(), 1 ) );
	return 0;
}

int getEnv( deque<string> const& args, Evaluator&, int, int ofd ) {
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

int cmdFork( deque<string> const& args, Evaluator&, int, int ofd ) {
	// XXX
	pid_t pid = forkExec( args, 0, 1 );
	ostringstream buf;
	buf << pid << '\n';
	writeAll( ofd, buf.str() );
	return 0;
}

int join( deque<string> const& args, Evaluator& eval, int, int ) {
	static_assert( sizeof( thread::id ) == sizeof( unsigned ), "" );

	for( auto const& arg: args ) {
		union U {
			U() {}
			thread::id tid;
			unsigned val;
		} u;
		u.val = readValue<unsigned>( arg );

		eval.mutexGlobal.lock();
		auto it = eval.backgrounds.find( u.tid );
		if( it != eval.backgrounds.end() ) {
			thread thr = move( it->second );
			eval.backgrounds.erase( it );
			eval.mutexGlobal.unlock();

			thr.join();
		}
		else {
			eval.mutexGlobal.unlock();
		}
	}
	return 0;
}

int wait( deque<string> const& args, Evaluator&, int, int ) {
	int retv = 0;
	for( auto const& arg: args ) {
		pid_t pid = readValue<int>( arg );
		int status;
		checkSysCall( waitpid( pid, &status, 0 ) );
		retv = WEXITSTATUS( status );
	}
	return retv;
}

int showList( deque<string> const& args, Evaluator&, int, int ofd ) {
	struct winsize ws;
	checkSysCall( ioctl( ofd, TIOCGWINSZ, &ws ) );

	int sum1 = 0;
	int sum2 = 0;
	for( auto it = args.cbegin(); it != args.cend(); ++it ) {
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

int strSize( deque<string> const& args, Evaluator&, int, int ofd ) {
	ostringstream buf;
	for( auto it = args.cbegin(); it != args.cend(); ++it ) {
		buf << it->size() << '\n';
	}

	writeAll( ofd, buf.str() );
	return 0;
}

int listSize( deque<string> const& args, Evaluator&, int, int ofd ) {
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
	map["%"] = cmdFork;
	map["sys.join"] = join;
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
