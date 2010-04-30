%{
	#include <exception>
	#include <string.h>
	#include "ast.hpp"
	#include "tokens.hpp"

	using namespace std;
%}

spc	[ \t\n]

%%

"("		{ return '('; }
")"		{ return ')'; }
"{"		{ return '{'; }
"}"		{ return '}'; }
"$"		{ return '$'; }

";"		{ return ';'; }
"&"		{ return '&'; }
"!"		{ return '!'; }
"|"		{ return '|'; }
"="		{ return '='; }
"^"		{ return '^'; }

"&&"	{ return AND2; }
"||"	{ return OR2; }
">|"	{ return RDFR; }
"|>"	{ return RDT1; }
"|>>"	{ return RDT2; }

[^(){}$" \t\n]+ {
	yylval.string = new basic_string<uint16_t>( yytext );
	return WORD;
}

\"(\\.|[^\\"])*\" {
	basic_string<uint16_t>* dst = new basic_string<uint16_t>();
	size_t len = strlen( yytext );
	for( size_t i = 1; i < len - 1; ++i ) {
		char c;
		if( yytext[i] == '\\' ) {
			++i;
			switch( yytext[i] ) {
				case 'a': c = '\a'; break;
				case 'b': c = '\b'; break;
				case 'f': c = '\f'; break;
				case 'n': c = '\n'; break;
				case 'r': c = '\r'; break;
				case 't': c = '\t'; break;
				case '"': c = '"'; break;
				case '\\': c = '\\'; break;
				default:
					throw exception();
			}
		}
		else {
			c = yytext[i];
		}
		dst->insert( dst->end(), c );
	}

	yylval.string = dst;
	return WORD;
}

[\t\n ]

%%
