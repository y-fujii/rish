%{
	#include "lex.hpp"

	namespace lex {
		char* bufCur;
		char* bufEnd;
	}

	using namespace lex;
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
<<EOF>>	{ return EOS; }

[^(){}$* \t\n]+ {
	yylval = new ast::Word( yytext );
	return WORD;
}

[\t\n ]

%%
