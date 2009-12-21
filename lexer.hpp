
#include "ast.hpp"
#include "tokens.hpp"

namespace ast {


Statement* parse( char const* bgn, char const* end ) {
	// XXX
	yy_scan_bytes( bgn, end - bgn );
	yyparse();

	return ::yylval;
}


}
