#pragma once

#include "ast.hpp"
#include "misc.hpp"
#include "tokens.hpp"
#include "lexer.hpp"


extern ast::Stmt* parseResult;
int yyparse();

ast::Stmt* parse( char const* bgn, char const* end ) {
	parseResult = NULL;
	yy_scan_bytes( const_cast<char*>( bgn ), end - bgn );
	yyparse();
	assert( parseResult != NULL );

	return parseResult;
}
