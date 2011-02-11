%{
	#include <exception>
	#include <string.h>
	#include "misc.hpp"
	#include "glob.hpp"
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
"@"		{ return '@'; }

"&&"	{ return TK_AND2; }
"||"	{ return TK_OR2; }
">|"	{ return TK_RDFR; }
"|>"	{ return TK_RDT1; }
"|>>"	{ return TK_RDT2; }
"|[0-9]+|"	{ return '|'; }
"|[0-9]+>"	{ return TK_RDT1; }
"|[0-9]+>>"	{ return TK_RDT2; }

"if"		{ return TK_IF; }
"else"		{ return TK_ELSE; }
"while"		{ return TK_WHILE; }
"for"		{ return TK_FOR; }
"break"		{ return TK_BREAK; }
"return"	{ return TK_RETURN; }
"let"		{ return TK_LET; }
"fun"		{ return TK_FUN; }

\$[a-zA-Z0-9_]+ {
	yylval.var = new std::string( yytext + 1 );
	return TK_VAR;
}

[^(){}$" \t\n]+ {
	yylval.string = new MetaString( yytext );
	for( MetaString::iterator it = yylval.string->begin(); it != yylval.string->end(); ++it ) {
		if( *it == '*' || *it == '?' ) {
			*it |= metaMask;
		}
	}
	return TK_WORD;
}

\"(\\.|[^\\"])*\" {
	MetaString* dst = new MetaString();
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
	return TK_WORD;
}

[ \t\n]+

%%
