// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license

#include <memory>
#include <iostream>
#include <fstream>
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
}

int main( int argc, char** argv ) {
	using namespace std;

	int opt;
	while( opt = getopt( argc, argv, "" ), opt != -1 ) {
	}

	struct sigaction sa;
	memset( &sa, 0, sizeof( sa ) );

	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_RESTART;
	sigaction( SIGPIPE, &sa, NULL );

	ThreadSupport::setup();

	if( optind < argc ) {
		while( optind < argc ) {
			try {
				ifstream ofs( argv[optind] );
				ast::Stmt* ast = parse( ofs );

				Evaluator eval;
				builtins::register_( eval.builtins );
				auto local = make_shared<Evaluator::Local>();
				eval.evalStmt( ast, local );
				eval.join();
			}
			catch( SyntaxError const& err ) {
				cerr << "Syntax error on " << argv[optind] << ":" << err.line + 1 << "." << endl;
				return 1;
			}

			++optind;
		}
	}
	else if( !isatty( 0 ) ) {
		try {
			ast::Stmt* ast = parse( cin );

			Evaluator eval;
			builtins::register_( eval.builtins );
			auto local = make_shared<Evaluator::Local>();
			eval.evalStmt( ast, local );
			eval.join();
		}
		catch( SyntaxError const& err ) {
			cerr << "Syntax error on #" << err.line + 1 << "." << endl;
			return 1;
		}
	}
	else {
		struct sigaction sa;
		memset( &sa, 0, sizeof( sa ) );

		sa.sa_handler = handleSigTSTP;
		sa.sa_flags = SA_RESTART;
		sigaction( SIGTSTP, &sa, NULL );

		sa.sa_handler = handleSigINT;
		sa.sa_flags = 0;
		sigaction( SIGINT, &sa, NULL );

		Evaluator eval;
		builtins::register_( eval.builtins );
		auto local = make_shared<Evaluator::Local>();
		while( true ) {
			char* line = readline( "| " );
			if( line == NULL ) break;
			auto deleter = scopeExit( bind( free, line ) );

			size_t len = strlen( line );
			if( len > 0 ) {
				add_history( line );
			}

			try {
				istringstream istr( line );
				ast::Stmt* ast = parse( istr );

				int retv = eval.evalStmt( ast, local );
				if( retv != 0 ) {
					cerr << "The command returned " << retv << "." << endl;
				}
			}
			catch( SyntaxError const& ) {
				cerr << "Syntax error." << endl;
			}
			catch( ThreadSupport::Interrupt const& ) {
				cerr << "\nInterrupted." << endl;
			}
			catch( IOError const& ) {
				cerr << "I/O error." << endl;
			}
			catch( ios_base::failure const& ) {
				cerr << "I/O error." << endl;
			}
		}

		eval.join();
	}

	return 0;
}
