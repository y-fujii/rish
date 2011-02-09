
#include <exception>
#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "ast.hpp"
#include "parser.hpp"
#include "eval.hpp"


int main( int argc, char** ) {
	// XXX
	assert( argc == 1 );

	deque<string> val;
	val.push_back( "aaa" );
	val.push_back( "bbb" );
	Global global;
	global.vars.insert( make_pair( "TEST", val ) );
	while( true ) {
		char* line = readline( "| " );
		if( line == NULL ) break;
		if( strlen( line ) > 0 ) {
			add_history( line );
		}
		try {
			ast::Statement* ast = parse( line, line + strlen( line ) );
			assert( ast != NULL );
			int retv = evalStatement( ast, &global );
			if( retv != 0 ) {
				cerr << "The command returned " << retv << "." << endl;
			}
		}
		catch( ... ) {
			free( line );
			throw;
		}
		free( line );
	}

	return 0;
}
