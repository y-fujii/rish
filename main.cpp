
#include <memory>
#include <iostream>
#include <cassert>
#include <readline/readline.h>
#include <readline/history.h>
#include "ast.hpp"
#include "parser.hpp"
#include "eval.hpp"


int main( int argc, char** ) {
	using namespace std;
	assert( argc == 1 );

	if( isatty( 0 ) ) {
		Global global;
		while( true ) {
			auto_ptr<char> line( readline( "| " ) );
			if( line.get() == NULL ) break;
			size_t len = strlen( line.get() );
			if( len > 0 ) {
				add_history( line.get() );
			}

			try {
				ast::Stmt* ast = parse( line.get(), line.get() + len );
				int retv = evalStmt( ast, &global, 0, 1 );
				if( retv != 0 ) {
					cerr << "The command returned " << retv << "." << endl;
				}
			}
			catch( SyntaxError const& ) {
				cerr << "Syntax error." << endl;
			}
			catch( RuntimeError const& ) {
				cerr << "Runtime error." << endl;
			}
			catch( IOError const& ) {
				cerr << "I/O error." << endl;
			}
			catch( ios_base::failure const& ) {
				cerr << "I/O error." << endl;
			}
		}
	}
	else {
		string buf(
			(istreambuf_iterator<char>( cin )),
			(istreambuf_iterator<char>())
		);

		ast::Stmt* ast = parse( buf.data(), buf.data() + buf.size() );

		Global global;
		evalStmt( ast, &global, 0, 1 );
	}

	return 0;
}
