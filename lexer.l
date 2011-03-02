/* (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license */

%{
	#include <iostream>
	#include <exception>
	#include <cassert>
	#include <string.h>
	#include "misc.hpp"
	#include "glob.hpp"
	#include "ast.hpp"
	#include "tokens.hpp"
	#include "parser.hpp"

	using namespace std;

	istream* parseIStream;
	size_t parseLineNo;


	#define YY_INPUT( buf, result, bufSize ) {		\
		parseIStream->read( buf, bufSize );			\
		result = parseIStream->gcount();			\
	}

	extern "C" int yywrap() {
		return 1;
	}
%}

%s state1
%%

"("			{ return '('; }
")"			{ return ')'; }
"{"			{ return '{'; }
"}"			{ return '}'; }
"$"			{ return '$'; }

";"			{ return ';'; }
"\n"		{ ++parseLineNo; return ';'; }
"&"			{ return '&'; }
"!"			{ return '!'; }
"|"			{ return '|'; }
"="			{ return '='; }
"^"			{ return '^'; }
"@"			{ return '@'; }

"&&"		{ return TK_AND2; }
"||"		{ return TK_OR2; }
">|"		{ return TK_RDFR; }
"|>"		{ return TK_RDT1; }
"|>>"		{ return TK_RDT2; }

"if"		{ return TK_IF; }
"else"		{ return TK_ELSE; }
\n[ \t]*else	{ ++parseLineNo; return TK_ELSE; }
"while"		{ return TK_WHILE; }
"for"		{ return TK_FOR; }
"break"		{ return TK_BREAK; }
"return"	{ return TK_RETURN; }
"let"		{ return TK_LET; }
"fun"		{ return TK_FUN; }
"when"		{ return TK_WHEN; }
"fetch"		{ return TK_FETCH; }

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
					case '0': c = '\0';   break;
					case 'a': c = '\a';   break;
					case 'b': c = '\b';   break;
					case 'f': c = '\f';   break;
					case 'n': c = '\n';   break;
					case 'r': c = '\r';   break;
					case 't': c = '\t';   break;
					case 'v': c = '\v';   break;
					case 'e': c = '\x1b'; break;
					default:
						delete dst;
						throw SyntaxError( parseLineNo );
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

[ \t]+

%%
