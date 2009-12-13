%{
	#include "ast.hpp"

	#define YYSTYPE Expr*
%}

%token AND2 OR2 RDT1 RDT2 RDFR EOF WORD
%token IF ELSE WHILE FOR BREAK RETURN LET FUN

%start command_list

%%

command_list
	: command_list ';' command_bg
	| ';' EOF
	| EOF

command_bg
	: command_andor '&' WORD
	| command_andor '&'
	| command_andor

command_andor
	: command_andor AND2 command_not
	| command_andor OR2 command_not
	| command_not

command_not
	: '!' command_pipe
	| command_pipe

command_pipe
	: command_pipe '|' command_stat
	| command_pipe RDT1 word
	| command_pipe RDT2 word
/*	| WORD RDFR command_pipe /* XXX */
	| command_stat

command_stat
	: if
	| WHILE command_andor '{' command_list '}' else
	| FOR WORD            '{' command_list '}' else
	| BREAK arg
	| RETURN arg
	| LET var_list '=' arg_list
	| FUN var_list '{' command_list '}'
	| '{' command_list '}'
	| WORD arg_list

if
	: IF command_andor '{' command_list '}' else

else
	: ELSE '{' command_list '}'
	| ELSE if
	| /* empty */

var_list
	: WORD var_list
	| '*' WORD
	| /* empty */

arg_list
	: arg_list arg_concat
	| arg_concat

arg_concat
	: arg_concat '^' arg
	| arg

arg
	: word
	| '$' '{' command_list '}'
	| '$' word
	| '(' arg_list ')'

word
	: WORD
	| IF
	| ELSE
	| WHILE
	| FOR
	| BREAK
	| RETURN
	| LET
	| FUN
