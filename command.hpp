#pragma once

#include <deque>
#include <iostream>
#include <cassert>
#include <unistd.h>
#include "misc.hpp"
#include "glob.hpp"


int runCommand( std::deque<std::string> const& args ) {
	using namespace std;

	assert( args.size() > 0 );

	if( args[0] == "cd" ) {
		if( args.size() != 2 ) {
			throw exception();
		}
		return chdir( args[1].c_str() ) < 0 ? 1 : 0;
	}
	else if( args[0] == "send" || args[0] == "yield" ) {
		for( deque<string>::const_iterator it = args.begin() + 1; it != args.end(); ++it ) {
			cout << *it << '\n';
		}
		return 0;
	}
	else {
		char const** args_raw = new char const*[args.size() + 1];
		for( size_t i = 0; i < args.size(); ++i ) {
			args_raw[i] = args[i].c_str();
		}
		args_raw[args.size()] = NULL;

		pid_t pid = fork();
		if( pid < 0 ) {
			return 1;
		}
		else if( pid == 0 ) {
			// XXX
			execvp( args_raw[0], const_cast<char* const*>( args_raw ) );
			return 1;
		}
		int status;
		if( waitpid( pid, &status, 0 ) < 0 ) {
			return 1;
		}
		return WEXITSTATUS( status );
	}
}
