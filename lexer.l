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
	bool parseAutoCaret;


	extern "C" int yywrap() {
		return 1;
	}

	#define YY_INPUT( buf, result, bufSize ) {		\
		parseIStream->read( buf, bufSize );			\
		result = parseIStream->gcount();			\
	}

	#define AUTO_CARET_END() { \
		if( parseAutoCaret ) { \
			parseAutoCaret = false; \
			string tmp( yytext ); \
			for( string::reverse_iterator i = tmp.rbegin(); i != tmp.rend(); ++i ) { \
				unput( *i ); \
			} \
			return '^'; \
		} \
	}

	#define AUTO_CARET_BGN() { \
		parseAutoCaret = true; \
	}
	
	#define AUTO_CARET_IGN() { \
		parseAutoCaret = false; \
	}
%}

%%

"("				{ AUTO_CARET_END(); return '('; }
")"				{ AUTO_CARET_BGN(); return ')'; }
"{"				{ AUTO_CARET_IGN(); return '{'; }
"}"				{ AUTO_CARET_IGN(); return '}'; }

";"				{ AUTO_CARET_IGN(); return ';'; }
"\n"			{ AUTO_CARET_IGN(); ++parseLineNo; return ';'; }
"&"				{ AUTO_CARET_IGN(); return '&'; }
"!"				{ AUTO_CARET_IGN(); return '!'; }
"|"				{ AUTO_CARET_IGN(); return '|'; }
"|"[ \t]*\n		{ AUTO_CARET_IGN(); ++parseLineNo; return '|'; }
"="				{ AUTO_CARET_IGN(); return '='; }
"^"				{ AUTO_CARET_IGN(); return '^'; }
"$"				{ AUTO_CARET_IGN(); return '$'; }

"&&"			{ AUTO_CARET_IGN(); return TK_AND2; }
"&&"[ \t]*\n	{ AUTO_CARET_IGN(); ++parseLineNo; return TK_AND2; }
"||"			{ AUTO_CARET_IGN(); return TK_OR2; }
"||"[ \t]*\n	{ AUTO_CARET_IGN(); ++parseLineNo; return TK_OR2; }
">|"			{ AUTO_CARET_IGN(); return TK_RDFR; }
"|>"			{ AUTO_CARET_IGN(); return TK_RDT1; }
"|>>"			{ AUTO_CARET_IGN(); return TK_RDT2; }

"if"			{ AUTO_CARET_IGN(); return TK_IF; }
"else"			{ AUTO_CARET_IGN(); return TK_ELSE; }
\n[ \t]*"else"	{ AUTO_CARET_IGN(); ++parseLineNo; return TK_ELSE; }
"while"			{ AUTO_CARET_IGN(); return TK_WHILE; }
"break"			{ AUTO_CARET_IGN(); return TK_BREAK; }
"return"		{ AUTO_CARET_IGN(); return TK_RETURN; }
"let"			{ AUTO_CARET_IGN(); return TK_LET; }
"fun"			{ AUTO_CARET_IGN(); return TK_FUN; }
"fetch"			{ AUTO_CARET_IGN(); return TK_FETCH; }
"yield"			{ AUTO_CARET_IGN(); return TK_YIELD; }
"defer"			{ AUTO_CARET_IGN(); return TK_DEFER; }

\$[a-zA-Z0-9_]+ {
	AUTO_CARET_END();
	AUTO_CARET_BGN();
	yylval.var = new string( yytext + 1 );
	return TK_VAR;
}

[^()$" \t\n]+ {
	AUTO_CARET_END();
	AUTO_CARET_BGN();
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
	AUTO_CARET_IGN();
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

[ \t]+ { AUTO_CARET_IGN(); }

%%
