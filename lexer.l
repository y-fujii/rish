%{
	#include <exception>
	#include <cassert>
	#include <string.h>
	#include "misc.hpp"
	#include "glob.hpp"
	#include "ast.hpp"
	#include "tokens.hpp"

	using namespace std;
%}

%%

"("		{ return '('; }
")"		{ return ')'; }
"{"		{ return '{'; }
"}"		{ return '}'; }
"$"		{ return '$'; }

";"		{ return ';'; }
"\n"	{ return ';'; }
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

"if"		{ return TK_IF; }
"else"		{ return TK_ELSE; }
"while"		{ return TK_WHILE; }
"for"		{ return TK_FOR; }
"break"		{ return TK_BREAK; }
"return"	{ return TK_RETURN; }
"let"		{ return TK_LET; }
"fun"		{ return TK_FUN; }

\$[a-zA-Z0-9_]+ {
	yylval.var = new string( yytext + 1 );
	return TK_VAR;
}

[^(){}$" \t\n]+ {
	yylval.word = new MetaString( yytext );
	MetaString::iterator it = yylval.word->begin();
	assert( it != yylval.word->end() );
	if( *it == '~' ) {
		*it |= metaMask;
	}
	while( it != yylval.word->end() ) {
		if( *it == '*' || *it == '?' ) {
			*it |= metaMask;
		}
		++it;
	}
	return TK_WORD;
}

\"(\\.|[^\\"])*\" {
	MetaString* dst = new MetaString();
	size_t len = strlen( yytext );
	assert( len >= 2 );
	for( size_t i = 1; i < len - 1; ++i ) {
		char c;
		if( yytext[i] == '\\' ) {
			++i;
			assert( yytext[i] != '\0' );
			if( isalnum( yytext[i] ) ) {
				switch( yytext[i] ) {
					MATCH( '0' ) { c = '\0'; }
					MATCH( 'a' ) { c = '\a'; }
					MATCH( 'b' ) { c = '\b'; }
					MATCH( 'f' ) { c = '\f'; }
					MATCH( 'n' ) { c = '\n'; }
					MATCH( 'r' ) { c = '\r'; }
					MATCH( 't' ) { c = '\t'; }
					MATCH( 'v' ) { c = '\v'; }
					OTHERWISE { throw SyntaxError(); }
				}
			}
			else {
				c = yytext[i];
			}
		}
		else {
			c = yytext[i];
		}
		dst->push_back( c );
	}

	yylval.word = dst;
	return TK_WORD;
}

#.*\n
[ \t]+

%%
