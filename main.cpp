
#include <iostream>
#include <assert.h>
#include <histedit.h>
#include "ast.hpp"
#include "parser.hpp"


int main( int argc, char** argv ) {
	FILE* stdin = fdopen( 0, "r" );
	FILE* stderr = fdopen( 1, "w" );
	FILE* stdout = fdopen( 2, "w" );
	EditLine* el = el_init( argv[0], stdin, stdout, stderr );

	while( true ) {
		int cnt = 0;
		char const* line = el_gets( el, &cnt );
		lex::init( line, line + cnt );
		Ast* ast = yyparse();
	}

	el_end( el );

	return 0;
}
