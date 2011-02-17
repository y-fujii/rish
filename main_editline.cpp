
#include <exception>
#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <histedit.h>
#include "ast.hpp"
#include "parser.hpp"
#include "eval.hpp"


char const* prompt( EditLine* ) {
	return "| ";
}

int main( int argc, char** argv ) {
	// XXX
	assert( argc == 1 );

	EditLine* el = el_init( argv[0], stdin, stdout, stderr );
	if( el == NULL ) throw std::exception();
	History* hs = history_init();
	if( hs == NULL ) throw std::exception();
	HistEvent he;

	history( hs, &he, H_SETSIZE, 4096 );
	//el_set( el, EL_ADDFN, "complete", "", _el_fn_complete );
	//el_set( el, EL_BIND, "\t", "complete" );
	el_set( el, EL_HIST, history, hs );
	el_set( el, EL_PROMPT, prompt );

	try {
		while( true ) {
			int cnt = 0;
			char const* line = el_gets( el, &cnt );
			if( line == NULL ) break;
			history( hs, &he, H_ENTER, line );

			ast::Statement* ast = parse( line, line + cnt );
			assert( ast != NULL );
			evalStatement( ast );
		}
	}
	catch( ... ) {
		el_end( el );
		history_end( hs );
		throw;
	}
	el_end( el );
	history_end( hs );

	return 0;
}
