// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include "config.hpp"
#include <deque>
#include <cassert>
#include <sstream>
#include <unistd.h>
#include "misc.hpp"
#include "glob.hpp"

using namespace std;


int runCommand( deque<string> const& args, int ifd, int ofd ) {
	assert( args.size() >= 1 );

	if( args[0] == "std.cd" ) {
		if( args.size() != 2 ) {
			return 1;
		}
		return chdir( args[1].c_str() ) < 0 ? 1 : 0;
	}
	else if( args[0] == "yield" ) {
		ostringstream buf;
		for( deque<string>::const_iterator it = args.begin() + 1; it != args.end(); ++it ) {
			buf << *it << '\n';
		}
		// XXX
		return write( ofd, buf.str().data(), buf.str().size() ) < 0 ? 1 : 0;
	}
	else {
		char const** args_raw = new char const*[args.size() + 1];
		for( size_t i = 0; i < args.size(); ++i ) {
			args_raw[i] = args[i].c_str();
		}
		args_raw[args.size()] = NULL;

		pid_t pid = fork();
		if( pid < 0 ) {
			throw IOError();
		}
		else if( pid == 0 ) {
			dup2( ifd, 0 );
			dup2( ofd, 1 );
			closefrom( 3 );
			execvp( args_raw[0], const_cast<char* const*>( args_raw ) );
			_exit( 1 );
		}

		int status;
		if( waitpid( pid, &status, 0 ) < 0 ) {
			throw IOError();
		}
		return WEXITSTATUS( status );
	}
}
