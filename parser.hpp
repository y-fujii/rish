#pragma once

#include <tr1/tuple>
#include "ast.hpp"
#include "tokens.hpp"
#include "lexer.hpp"


int yyparse();

namespace ast {

Statement* parse( char const* bgn, char const* end ) {
	::yy_scan_bytes(
		const_cast<char*>( bgn ),
		end - bgn
	);
	::yyparse();

	return ::yylval.statement;
}

}
