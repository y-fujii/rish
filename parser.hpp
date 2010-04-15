#pragma once

#include "ast.hpp"
#include "tokens.hpp"
#include "lexer.hpp"


extern ast::Statement* parse_result;
int yyparse();

ast::Statement* parse( char const* bgn, char const* end ) {
	yy_scan_bytes( const_cast<char*>( bgn ), end - bgn );
	yyparse();

	return parse_result;
}
