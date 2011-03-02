// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license

#include "config.hpp"
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

using namespace std;


atomic<bool> stop( false );

__thread void (*threadSigHandler)( int ) = nullptr;

void handleSignal( int s ) {
	if( threadSigHandler != nullptr ) {
		threadSigHandler( s );
	}
}

void handleSigTSTP( int ) {
	/*
	setpgid( 0, 0 );
	*/
}

// XXX
void handleSigINT( int ) {
	char data[] = "\r\x1b[K";
	write( 0, data, size( data ) );
	rl_initialize();
	rl_redisplay();

	stop.store( true );
}

int main( int argc, char** ) {
	using namespace std;
	assert( argc == 1 );

	if( isatty( 0 ) ) {
		struct sigaction sa;
		memset( &sa, 0, sizeof( sa ) );

		sa.sa_handler = handleSigTSTP;
		sa.sa_flags = SA_RESTART;
		sigaction( SIGTSTP, &sa, NULL );

		sa.sa_handler = handleSigINT;
		sa.sa_flags = 0;
		sigaction( SIGINT, &sa, NULL );

		Global global;
		while( true ) {
			char* line = readline( "| " );
			try {
				if( line == NULL ) break;
				size_t len = strlen( line );
				if( len > 0 ) {
					add_history( line );
				}

				try {
					istringstream istr( line );
					ast::Stmt* ast = parse( istr );

					stop.store( false );
					int retv = evalStmt( ast, &global, nullptr, 0, 1, stop );
					if( retv != 0 ) {
						cerr << "The command returned " << retv << "." << endl;
					}
				}
				catch( SyntaxError const& ) {
					cerr << "Syntax error." << endl;
				}
				catch( RuntimeError const& ) {
					cerr << "Runtime error." << endl;
				}
				catch( StopException const& ) {
					cerr << "\nInterrupted." << endl;
				}
				catch( IOError const& ) {
					cerr << "I/O error." << endl;
				}
				catch( ios_base::failure const& ) {
					cerr << "I/O error." << endl;
				}
			}
			catch( ... ) {
				free( line );
				throw;
			}
			free( line );
		}
	}
	else {
		ast::Stmt* ast;
		try {
			ast = parse( cin );
		}
		catch( SyntaxError const& err ) {
			cerr << "Syntax error on #" << err.line + 1 << "." << endl;
			return 1;
		}

		Global global;
		stop.store( false );
		evalStmt( ast, &global, nullptr, 0, 1, stop );
	}

	return 0;
}
