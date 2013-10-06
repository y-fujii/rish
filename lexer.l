/* (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license */

%{
	#include <cassert>
	#include <exception>
	#include <iostream>
	#include <memory>
	#include <string.h>
	#include "misc.hpp"
	#include "glob.hpp"
	#include "ast.hpp"
	#include "parser.hpp"
	#include "tokens.hpp"

	using namespace std;

	istream* lexerIStream = nullptr;
	size_t   lexerLineNo  = 0;

	#define YY_INPUT( buf, result, bufSize ) {		\
		lexerIStream->read( buf, bufSize );			\
		result = lexerIStream->gcount();			\
	}

	#pragma GCC diagnostic ignored "-Wsign-compare"
	#pragma GCC diagnostic ignored "-Wunused-function"
%}

/* [N]ormal, auto [C]aret, [I]gnore newline */
%x N C I

%%

<N,I>"("				{ BEGIN( I ); return '('; }
<N,I>"["				{ BEGIN( I ); return '['; }
<N,C,I>")"				{ BEGIN( C ); return ')'; }
<N,C,I>"]"				{ BEGIN( C ); return ']'; }
<N,I>"{"				{ BEGIN( I ); return '{'; }
<N,I>"}"				{ BEGIN( N ); return '}'; }

<N,I>";"				{ BEGIN( I ); return ';'; }
<N,I>":"				{ BEGIN( I ); return ':'; }
<N,I>"&"				{ BEGIN( I ); return '&'; }
<N,I>"!"				{ BEGIN( I ); return '!'; }
<N,I>"|"				{ BEGIN( I ); return '|'; }
<N,I>"="				{ BEGIN( I ); return '='; }
<N,I>"^"				{ BEGIN( I ); return '^'; }
<N,I>"$"				{ BEGIN( I ); return '$'; }
<N,I>"&&"				{ BEGIN( I ); return TK_AND2; }
<N,I>"||"				{ BEGIN( I ); return TK_OR2; }
<N,I>"->"				{ BEGIN( I ); return TK_ARROW; }
<N,I>">|"				{ BEGIN( I ); return TK_RDFR; }
<N,I>"|>"				{ BEGIN( I ); return TK_RDT1; }
<N,I>"|>>"				{ BEGIN( I ); return TK_RDT2; }

<N,I>"~"				{ BEGIN( C ); return '~'; }
<N,I>"+"				{ BEGIN( C ); return '+'; }
<N,I>"-"				{ BEGIN( C ); return '-'; }
<N,I>"*"				{ BEGIN( C ); return '*'; }
<N,I>"/"				{ BEGIN( C ); return '/'; }
<N,I>"%"				{ BEGIN( C ); return '%'; }
<N,I>"<"				{ BEGIN( C ); return '<'; }
<N,I>">"				{ BEGIN( C ); return '>'; }
<N,I>"=="				{ BEGIN( C ); return TK_EQ; }
<N,I>"!="				{ BEGIN( C ); return TK_NE; }
<N,I>"<="				{ BEGIN( C ); return TK_LE; }
<N,I>">="				{ BEGIN( C ); return TK_GE; }

<N,I>"if"				{ BEGIN( N ); return TK_IF; }
<N,I>"else"				{ BEGIN( N ); return TK_ELSE; }
<N,I>\n[ \t]*"else"		{ BEGIN( N ); ++lexerLineNo; return TK_ELSE; }
<N,I>"while"			{ BEGIN( N ); return TK_WHILE; }
<N,I>"break"			{ BEGIN( N ); return TK_BREAK; }
<N,I>"return"			{ BEGIN( N ); return TK_RETURN; }
<N,I>"let"				{ BEGIN( N ); return TK_LET; }
<N,I>"fun"				{ BEGIN( N ); return TK_FUN; }
<N,I>"fetch"			{ BEGIN( N ); return TK_FETCH; }
<N,I>"yield"			{ BEGIN( N ); return TK_YIELD; }
<N,I>"zip"				{ BEGIN( N ); return TK_ZIP; }
<N,I>"defer"			{ BEGIN( N ); return TK_DEFER; }
<N,I>"chdir"			{ BEGIN( N ); return TK_CHDIR; }

<N,I>\$[a-zA-Z0-9_]+ {
	yylval.var = new ast::Var( string( yytext + 1, yyleng - 1 ), lexerLineNo );
	BEGIN( C );
	return TK_VAR;
}

<N,I>#[a-zA-Z0-9_]+ {
	yylval.var = new ast::Var( string( yytext + 1, yyleng - 1 ), lexerLineNo );
	BEGIN( C );
	return TK_SIZE;
}

<N,I>\$[a-zA-Z0-9_]+\( {
	yylval.var = new ast::Var( string( yytext + 1, yyleng - 2 ), lexerLineNo );
	BEGIN( I );
	return TK_INDEX;
}

<N,I>[^~()\[\]$" \t\n][^()\[\]$" \t\n]* {
	MetaString w( yytext, yytext + yyleng );
	for( auto& c: w ) {
		if( c == '*' || c == '?' ) {
			c |= metaMask;
		}
	}

	yylval.word = new ast::Word( move( w ) );
	BEGIN( C );
	return TK_WORD;
}

<N,I>\"(\\.|[^\\"])*\" {
	MetaString w;
	w.reserve( yyleng );
	for( int i = 1; i < yyleng - 1; ++i ) {
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
						throw SyntaxError( lexerLineNo );
				}
			}
			else {
				c = yytext[i];
			}
		}
		else {
			c = yytext[i];
		}
		w.push_back( c );
	}

	yylval.word = new ast::Word( move( w ) );
	BEGIN( N );
	return TK_WORD;
}

<C>[^\)\] \t\n]		{ BEGIN( N ); yyless( 0 ); return '^'; }
<I>[ \t]+			{}
<N,C>[ \t]+			{ BEGIN( N ); }
<I>"\n"				{ ++lexerLineNo; }
<N,C>"\n"			{ BEGIN( N ); ++lexerLineNo; return ';'; }

<I>"//"[^\n]*		{}
<N,C>"//"[^\n]*		{ BEGIN( N ); }

%%

extern "C" int yywrap() {
	return 1;
}

void lexerInit( istream* istr ) {
	lexerIStream = istr;
	lexerLineNo  = 0;
	BEGIN( N );
}

size_t lexerGetLineNo() {
	return lexerLineNo;
}
