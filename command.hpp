#pragma once

#include <deque>
#include <unistd.h>


void runCommand( std::deque<std::string> const& args ) {
	using namespace std;

	if( args[0] == "cd" ) {
		if( args.size() != 2 ) {
			throw exception();
		}
		if( chdir( args[1].c_str() ) < 0 ) {
			throw exception();
		}
	}
	else {
		char const** args_raw = new char const*[args.size() + 1];
		for( size_t i = 0; i < args.size(); ++i ) {
			args_raw[i] = args[i].c_str();
		}
		args_raw[args.size()] = NULL;

		pid_t pid = fork();
		if( pid < 0 ) {
			throw exception();
		}
		else if( pid == 0 ) {
			// XXX
			execvp( args_raw[0], const_cast<char* const*>( args_raw ) );
			throw exception();
		}
		if( waitpid( pid, NULL, 0 ) < 0 ) {
			throw exception();
		}
	}
}
