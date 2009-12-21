
#include <exception>
#include <iostream>
#include <assert.h>
#include <stdio.h>
//#include <histedit.h>
#include "ast.hpp"
#include "parser.hpp"


int main( int argc, char** argv ) {
	EditLine* el = el_init( argv[0], stdin, stdout, stderr );
	if( el == NULL ) throw std::exception();

	while( true ) {
		int cnt = 0;
		char const* line = el_gets( el, &cnt );
		if( line == NULL ) throw std::exception();
		ast::Statement* ast = ast::parse( line, line + cnt );
	}

	el_end( el );

	return 0;
}
