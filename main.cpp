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
#include "annotate.hpp"
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
			ast::Var var( "args", -1 );
			Annotator::Local alocal;
			alocal.assign( &var );
			auto elocal = make_shared<Evaluator::Local>();
			elocal->vars.resize( alocal.vars.size() );
			copy( 
				&argv[optind + 1], &argv[argc],
				back_inserter( elocal->value( &var ) )
			);

			ifstream ifs( argv[optind] );
			unique_ptr<ast::Stmt> ast = parse( ifs );
			Annotator().annotate( ast.get(), alocal );
			Evaluator eval;
			eval.evalStmt( ast.get(), elocal, 0, 1 );
			eval.join();
		}
		catch( SyntaxError const& err ) {
			cerr << "Syntax error on #" << err.line + 1 << "." << endl;
			return 1;
		}
	}
	/*
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
		Repl();
	}
	*/

	return 0;
}
