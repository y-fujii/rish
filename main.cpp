
#include <exception>
#include <iostream>
#include <iterator>
#include <assert.h>
#include <stdio.h>
#include "ast.hpp"
#include "parser.hpp"
#include "eval.hpp"


int main( int argc, char** ) {
	using namespace std;
	assert( argc == 1 );

	string buf(
		(istreambuf_iterator<char>( cin )),
		(istreambuf_iterator<char>())
	);

	ast::Statement* ast = parse( buf.data(), buf.data() + buf.size() );
	assert( ast != NULL );

	Global global;
	int retv = evalStatement( ast, &global, 0, 1 );
	if( retv != 0 ) {
		cerr << "The command returned " << retv << "." << endl;
	}

	return 0;
}
