
#include <exception>
#include <iostream>
#include <assert.h>
#include <stdio.h>
#include "ast.hpp"
#include "parser.hpp"
#include "eval.hpp"


int main( int argc, char** ) {
	using namespace std;
	assert( argc == 1 );

	Global global;
	while( true ) {
		string line;
		getline( cin, line, '\0' );

		ast::Statement* ast = parse( line.data(), line.data() + line.size() );
		assert( ast != NULL );
		int retv = evalStatement( ast, &global, 0, 1 );
		if( retv != 0 ) {
			cerr << "The command returned " << retv << "." << endl;
		}
	}

	return 0;
}
