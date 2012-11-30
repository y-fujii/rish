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

	istream* lexerIStream;
	size_t lexerLineNo;

	#define YY_INPUT( buf, result, bufSize ) {		\
		lexerIStream->read( buf, bufSize );			\
		result = lexerIStream->gcount();			\
	}

	#define INSERT_CARET() {	\
		BEGIN( N );				\
		yyless( 0 );			\
		return '^';				\
	}
%}

/* [N]ormal, auto [C]aret, [I]gnore newline */
%x N C I

%%

<C>"("					{ INSERT_CARET(); }
<N,I>"("				{ BEGIN( I ); return '('; }
<N,C,I>")"				{ BEGIN( C ); return ')'; }
<N,C,I>"{"				{ BEGIN( I ); return '{'; }
<N,C,I>"}"				{ BEGIN( N ); return '}'; }

<N,C,I>";"				{ BEGIN( I ); return ';'; }
<N,C,I>"&"				{ BEGIN( I ); return '&'; }
<N,C,I>"!"				{ BEGIN( I ); return '!'; }
<N,C,I>"|"				{ BEGIN( I ); return '|'; }
<N,C,I>"="				{ BEGIN( N ); return '='; }
<N,C,I>"^"				{ BEGIN( I ); return '^'; }
<N,C,I>"$"				{ BEGIN( I ); return '$'; }

<N,C,I>"&&"				{ BEGIN( I ); return TK_AND2; }
<N,C,I>"||"				{ BEGIN( I ); return TK_OR2; }
<N,C,I>">|"				{ BEGIN( I ); return TK_RDFR; }
<N,C,I>"|>"				{ BEGIN( I ); return TK_RDT1; }
<N,C,I>"|>>"			{ BEGIN( I ); return TK_RDT2; }

<N,C,I>"if"				{ BEGIN( N ); return TK_IF; }
<N,C,I>"else"			{ BEGIN( N ); return TK_ELSE; }
<N,C,I>\n[ \t]*"else"	{ BEGIN( N ); ++lexerLineNo; return TK_ELSE; }
<N,C,I>"while"			{ BEGIN( N ); return TK_WHILE; }
<N,C,I>"break"			{ BEGIN( N ); return TK_BREAK; }
<N,C,I>"return"			{ BEGIN( N ); return TK_RETURN; }
<N,C,I>"let"			{ BEGIN( N ); return TK_LET; }
<N,C,I>"fun"			{ BEGIN( N ); return TK_FUN; }
<N,C,I>"fetch"			{ BEGIN( N ); return TK_FETCH; }
<N,C,I>"yield"			{ BEGIN( N ); return TK_YIELD; }
<N,C,I>"defer"			{ BEGIN( N ); return TK_DEFER; }

<C>\$[a-zA-Z0-9_]+ { INSERT_CARET(); }
<N,I>\$[a-zA-Z0-9_]+ {
	yylval.var = new string( yytext + 1, yyleng - 1 );
	BEGIN( C );
	return TK_VAR;
}

<C>\$[a-zA-Z0-9_]+\( { INSERT_CARET(); }
<N,I>\$[a-zA-Z0-9_]+\( {
	yylval.var = new string( yytext + 1, yyleng - 2 );
	BEGIN( I );
	return TK_ARRAY;
}

<C>[^()$" \t\n]+ { INSERT_CARET(); }
<N,I>[^()$" \t\n]+ {
	auto dst = make_unique<MetaString>( yytext, yytext + yyleng );
	auto it = dst->begin();
	assert( it != dst->end() );
	if( *it == '~' ) {
		*it |= metaMask;
	}
	while( it != dst->end() ) {
		if( *it == '*' || *it == '?' ) {
			*it |= metaMask;
		}
		++it;
	}

	yylval.word = dst.release();
	BEGIN( C );
	return TK_WORD;
}

<N,C,I>\"(\\.|[^\\"])*\" {
	auto dst = make_unique<MetaString>();
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
		dst->push_back( c );
	}

	yylval.word = dst.release();
	BEGIN( N );
	return TK_WORD;
}

<I>[ \t]+		{}
<N,C>[ \t]+		{ BEGIN( N ); }
<I>"\n"			{ ++lexerLineNo; }
<N,C>"\n"		{ BEGIN( N ); ++lexerLineNo; return ';'; }
<I>#[^\n]*		{}
<N,C>#[^\n]*	{ BEGIN( N ); }

%%

extern "C" int yywrap() {
	return 1;
}

void lexerInit( istream* istr ) {
	lexerLineNo = 0;
	lexerIStream = istr;
	BEGIN( N );
}

size_t lexerGetLineNo() {
	return lexerLineNo;
}
