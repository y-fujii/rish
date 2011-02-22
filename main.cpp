
#include "config.hpp"
#include <memory>
#include <tr1/memory>
#include <iostream>
#include <cassert>
#include <unistd.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "misc.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "eval.hpp"


struct ReplState {
	enum type {
		read, eval, null
	};
};

std::atomic<bool> stop( false );
volatile bool signaled = false;
volatile ReplState::type replState = ReplState::null;

void handleSigTSTP( int ) {
	setpgid( 0, 0 );
}

void handleSigINT( int ) {
	/*
	char data[] = "\r\x1b[K";
	write( 0, data, size( data ) );
	*/
	switch( replState ) {
		MATCH( ReplState::read ) {
		}
		MATCH( ReplState::eval ) {
			stop.store( true );
		}
		OTHERWISE {
			assert( false );
		}
	}
	signaled = true;
}

int main( int argc, char** ) {
	using namespace std;
	assert( argc == 1 );

	if( isatty( 0 ) ) {
		struct sigaction sa = {};

		sa.sa_handler = handleSigTSTP;
		sa.sa_flags = SA_RESTART;
		sigaction( SIGTSTP, &sa, NULL );

		sa.sa_handler = handleSigINT;
		sa.sa_flags = 0;
		sigaction( SIGINT, &sa, NULL );

		Global global;
		while( true ) {
			replState = ReplState::read;
			auto_ptr<char> const line( readline( "| " ) );
			if( line.get() == NULL ) break;
			size_t len = strlen( line.get() );
			if( len > 0 ) {
				add_history( line.get() );
			}

			signaled = false;

			replState = ReplState::eval;
			try {
				ast::Stmt* ast = parse( line.get(), line.get() + len );
				int retv = evalStmt( ast, &global, 0, 1, stop );
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
				cerr << "Interrupted." << endl;
				stop.store( false );
			}
			catch( IOError const& ) {
				cerr << "I/O error." << endl;
			}
			catch( ios_base::failure const& ) {
				cerr << "I/O error." << endl;
			}
			
			if( signaled ) {
				signaled = false;
				write( 0, "\n", 1 );
			}
		}
	}
	else {
		string buf(
			(istreambuf_iterator<char>( cin )),
			(istreambuf_iterator<char>())
		);

		ast::Stmt* ast = parse( buf.data(), buf.data() + buf.size() );

		atomic<bool> stop( false );
		Global global;
		evalStmt( ast, &global, 0, 1, stop );
	}

	return 0;
}
