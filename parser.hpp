// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <iostream>
#include "ast.hpp"
#include "misc.hpp"


extern std::istream* parseIStream;
extern size_t parseLineNo;
extern bool parseAutoCaret;
extern ast::Stmt* parseResult;
int yylex();
int yyparse();


struct SyntaxError: exception {
	SyntaxError( size_t l ): line( l ) {}

	size_t line;
};

inline ast::Stmt* parse( istream& istr ) {
	parseIStream = &istr;
	parseLineNo = 0;
	parseAutoCaret = false;
	parseResult = nullptr;
	yyparse();
	assert( parseResult != nullptr );

	return parseResult;
}

template<class Iter>
Iter lex( istream& istr, Iter dstIt ) {
	parseIStream = &istr;
	parseLineNo = 0;
	parseAutoCaret = false;
	char c;
	while( c = yylex(), c != 0 ) {
		*dstIt++ = c;
	}
	return dstIt;
}
