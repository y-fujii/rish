// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license

#include <memory>
#include <tr1/memory>
#include <iostream>
#include <cassert>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "misc.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "eval.hpp"
#include "builtins.hpp"

using namespace std;


void handleSigTSTP( int ) {
	/*
	setpgid( 0, 0 );
	*/
}

// XXX
void handleSigINT( int ) {
	string data( "\r\x1b[K" );
	writeAll( 0, data );
	rl_initialize();
	rl_redisplay();

	Thread::_interrupted() = true;
}

int main( int argc, char** ) {
	using namespace std;
	assert( argc == 1 );

	if( isatty( 0 ) ) {
		Thread::setup();

		struct sigaction sa;
		memset( &sa, 0, sizeof( sa ) );

		sa.sa_handler = handleSigTSTP;
		sa.sa_flags = SA_RESTART;
		sigaction( SIGTSTP, &sa, NULL );

		sa.sa_handler = handleSigINT;
		sa.sa_flags = 0;
		sigaction( SIGINT, &sa, NULL );

		Global global;
		builtins::register_( global.builtins );
		while( true ) {
			char* line = readline( "| " );
			if( line == NULL ) break;
			ScopeExit deleter( bind( free, line ) );

			size_t len = strlen( line );
			if( len > 0 ) {
				add_history( line );
			}

			try {
				istringstream istr( line );
				ast::Stmt* ast = parse( istr );

				Thread::_interrupted() = false;
				int retv = evalStmt( ast, &global, nullptr, 0, 1 );
				if( retv != 0 ) {
					cerr << "The command returned " << retv << "." << endl;
				}
			}
			catch( SyntaxError const& ) {
				cerr << "Syntax error." << endl;
			}
			catch( Thread::Interrupt const& ) {
				cerr << "\nInterrupted." << endl;
			}
			catch( IOError const& ) {
				cerr << "I/O error." << endl;
			}
			catch( ios_base::failure const& ) {
				cerr << "I/O error." << endl;
			}
		}
	}
	else {
		try {
			ast::Stmt* ast = parse( cin );
			Global global;
			Thread::_interrupted() = false;
			evalStmt( ast, &global, nullptr, 0, 1 );
		}
		catch( SyntaxError const& err ) {
			cerr << "Syntax error on #" << err.line + 1 << "." << endl;
			return 1;
		}
	}

	return 0;
}
