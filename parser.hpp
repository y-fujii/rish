// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <memory>
#include <iostream>
#include "ast.hpp"
#include "misc.hpp"


void lexerInit( istream* );
size_t lexerGetLineNo();
int yylex();
int yyparse();
extern unique_ptr<ast::Stmt> parserResult;


struct SyntaxError: exception {
	SyntaxError( size_t l ): line( l ) {}

	size_t line;
};

inline unique_ptr<ast::Stmt> parse( istream& istr ) {
	parserResult = nullptr;
	lexerInit( &istr );
	yyparse();
	assert( parserResult );

	return std::move( parserResult );
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

template<class T>
struct StupidPtr {
	// POD

	StupidPtr& operator=( T* p ) {
		_ptr = p;
		return *this;
	}

	operator unique_ptr<T>() {
		return unique_ptr<T>( _ptr );
	}

	private:
		T* _ptr;
};
