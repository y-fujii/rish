// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <iostream>
#include "ast.hpp"
#include "misc.hpp"


void lexerInit( istream* );
size_t lexerGetLineNo();
int yylex();
int yyparse();
extern ast::Stmt* parserResult;


struct SyntaxError: exception {
	SyntaxError( size_t l ): line( l ) {}

	size_t line;
};

inline ast::Stmt* parse( istream& istr ) {
	parserResult = nullptr;
	lexerInit( &istr );
	yyparse();
	assert( parserResult != nullptr );

	return parserResult;
}

template<class Iter>
Iter lex( istream& istr, Iter dstIt ) {
	lexerInit( &istr );
	int c;
	while( c = yylex(), c != 0 ) {
		*dstIt++ = c;
	}
	return dstIt;
}
