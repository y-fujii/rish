// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "misc.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "eval.hpp"
#include "repl.hpp"
//#include "builtins.hpp"

using namespace std;


int main( int argc, char** argv ) {
	int opt;
	while( opt = getopt( argc, argv, "" ), opt != -1 ) {
	}

	struct sigaction sa;
	memset( &sa, 0, sizeof( sa ) );
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_RESTART;
	sigaction( SIGPIPE, &sa, nullptr );

	if( optind < argc ) {
		try {
			ifstream ifs( argv[optind] );
			unique_ptr<ast::Stmt> ast = parse( ifs );

			Evaluator eval;
			auto local = make_shared<Evaluator::Local>();
			ast::Var var( "args" );
			copy( 
				&argv[optind + 1], &argv[argc],
				back_inserter( local->value( &var ) )
			);
			eval.evalStmt( ast.get(), local, 0, 1 );
			eval.join();
		}
		catch( SyntaxError const& err ) {
			cerr << "Syntax error on #" << err.line + 1 << "." << endl;
			return 1;
		}
	}
	else if( !isatty( 0 ) ) {
		try {
			unique_ptr<ast::Stmt> ast = parse( cin );

			Evaluator eval;
			auto local = make_shared<Evaluator::Local>();
			eval.evalStmt( ast.get(), local, 0, 1 );
			eval.join();
		}
		catch( SyntaxError const& err ) {
			cerr << "Syntax error on #" << err.line + 1 << "." << endl;
			return 1;
		}
	}
	else {
		Repl repl;
	}

	return 0;
}
