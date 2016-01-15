// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <cassert>
#include <cmath>
#include <vector>
#include <sstream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <sys/ioctl.h>
#include "misc.hpp"
#include "glob.hpp"
#include "eval.hpp"
#include "unix.hpp"

using namespace std;

namespace builtins {


int setEnv( vector<string> const& args, Evaluator&, int, int ) {
	if( args.size() != 2 ) {
		return 1;
	}
	checkSysCall( setenv( args[0].c_str(), args[1].c_str(), 1 ) );
	return 0;
}

int getEnv( vector<string> const& args, Evaluator&, int, int ofd ) {
	if( args.size() != 1 ) {
		return 1;
	}
	char* val = getenv( args[0].c_str() );
	if( val == nullptr ) {
		return 1;
	}
	writeAll( ofd, string( val ) + '\n' );
	return 0;
}

int join( vector<string> const& args, Evaluator& eval, int, int ) {
	static_assert( sizeof( thread::id ) == sizeof( uintptr_t ), "" );

	for( auto const& arg: args ) {
		union U {
			U() {}
			thread::id tid;
			uintptr_t val;
		} u;
		u.val = readValue<uintptr_t>( arg );

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

int wait( vector<string> const& args, Evaluator&, int, int ) {
	int retv = 0;
	for( auto const& arg: args ) {
		pid_t pid = readValue<int>( arg );
		int status;
		checkSysCall( waitpid( pid, &status, 0 ) );
		retv = WEXITSTATUS( status );
	}
	return retv;
}

template<class Map>
void register_( Map& map ) {
	map["sys.setenv"] = setEnv;
	map["sys.getenv"] = getEnv;
	map["sys.join"] = join;
	map["sys.wait"] = wait;
}


}
