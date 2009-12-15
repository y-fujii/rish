%{
	#include "ast.hpp"
	#include "parse.hpp"
	#include <string>

	#define YY_INPUT( buf, size, sizeMax ) { \
		int c = getchar(); \
		if( c == EOF ) { \
			size = YY_NULL; \
		} \
		else { \
			buf[0] = c; \
			size = 1; \
		} \
	}

	/*
	int yywrap() {
		return 1;
	}
	*/

	void lexInit() {
	}

	void lexTerm() {
	}
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
"*"		{ return '*'; }
"^"		{ return '^'; }

"&&"	{ return AND2; }
"||"	{ return OR2; }
">|"	{ return RDFR; }
"|>"	{ return RDT1; }
"|>>"	{ return RDT2; }
<<EOF>>	{ return EOF; }

[^(){}$* \t\n]+ {
	yylval = boost::shared_ptr< string >(
		new ast::Word( yytext )
	);
	return WORD;
}

%%
