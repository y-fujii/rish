
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

	Global global;
	while( true ) {
		char* line = readline( "| " );
		if( line == NULL ) break;
		if( strlen( line ) > 0 ) {
			add_history( line );
		}
		try {
			ast::Statement* ast = parse( line, line + strlen( line ) );
			assert( ast != NULL );
			int retv = evalStatement( ast, &global, 0, 1 );
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
