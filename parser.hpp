// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include "ast.hpp"
#include "misc.hpp"
#include "lexer.hpp"


extern size_t parseLineNo;
extern ast::Stmt* parseResult;
int yyparse();

inline ast::Stmt* parse( char const* bgn, char const* end ) {
	parseLineNo = 0;
	parseResult = NULL;
	yy_scan_bytes( const_cast<char*>( bgn ), end - bgn );
	yyparse();
	assert( parseResult != NULL );

	return parseResult;
}
