%{
	#include <exception>
	#include <string.h>
	#include "ast.hpp"
	#include "tokens.hpp"
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

[^(){}$*" \t\n]+ {
	yylval.string = new std::string( yytext );
	return WORD;
}

\"(\\.|[^\\"])*\" {
	std::string* dst = new std::string();
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
					throw std::exception();
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
