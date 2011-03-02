// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <iostream>
#include "ast.hpp"
#include "misc.hpp"


extern std::istream* parseIStream;
extern size_t parseLineNo;
extern ast::Stmt* parseResult;
int yylex();
int yyparse();

inline ast::Stmt* parse( istream& istr ) {
	parseIStream = &istr;
	parseLineNo = 0;
	parseResult = nullptr;
	yyparse();
	assert( parseResult != nullptr );

	return parseResult;
}

template<class Iter>
Iter lex( istream& istr, Iter dstIt ) {
	parseIStream = &istr;
	parseLineNo = 0;
	char c;
	while( c = yylex(), c != 0 ) {
		*dstIt++ = c;
	}
	return dstIt;
}
