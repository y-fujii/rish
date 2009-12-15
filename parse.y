%{
	#include "ast.hpp"

	#define YYSTYPE Expr*
%}

%token AND2 OR2 RDT1 RDT2 RDFR EOF WORD
%token IF ELSE WHILE FOR BREAK RETURN LET FUN

%start command_list

%%

command_list
	: command_list ';' command_bg		{ $$ = new ast::Seq( $1, $3 ); }
	| ';' EOF							{ $$ = NULL; }
	| EOF								{ $$ = NULL; }

command_bg
	: command_andor '&' WORD			{ $$ = new ast::Bg( $1, $3 ); }
	| command_andor '&'					{ $$ = new ast::Bg( $1, NULL ); }
	| command_andor

command_andor
	: command_andor AND2 command_not	{ $$ = new ast::And( $1, $3 ); }
	| command_andor OR2 command_not		{ $$ = new ast::Or( $1, $3 ); }
	| command_not

command_not
	: '!' command_pipe					{ $$ = new ast::Not( $2 ); }
	| command_pipe

command_pipe
	: command_pipe '|' command_stat
	| command_pipe RDT1 word
	| command_pipe RDT2 word
/*	| WORD RDFR command_pipe /* XXX */
	| command_stat

command_stat
	: if
	| WHILE command_andor '{' command_list '}' else	{ $$ = new ast::While( $2, $4, $6 ); }
	| FOR WORD            '{' command_list '}' else	{ $$ = new ast::For( $2, $4, $6 ); }
	| BREAK arg										{ $$ = new ast::Break( $2 ); }
	| RETURN arg									{ $$ = new ast::Return( $2 ); }
	| LET var_list var_star '=' arg_list			{ $$ = new ast::Let( $2, $3, $5 ); }
	| FUN var_list var_star '{' command_list '}'	{ $$ = new ast::Fun( $2, $3, $5 ); }
	| '{' command_list '}'							{ $$ = $2 };
	| WORD arg_list									{ $$ = new ast::Cmd( $1, $2 ); }

if
	: IF command_andor '{' command_list '}' else	{ $$ = ast::If( $2, $4, $6 ); }

else
	: ELSE '{' command_list '}'	{ $$ = $3; }
	| ELSE if					{ $$ = $2; }
	| /* empty */				{ $$ = NULL; }

var_list
	: WORD var_list				{ $$ = new ast::List( $1, $2 ); }
	| /* empty */				{ $$ = NULL; }

var_star
	: '*' WORD					{ $$ = new ast::Star( $2 ); }
	| /* empty */				{ $$ = NULL; }

arg_list
	: arg_concat arg_list		{ $$ = new ast::List( $1, $2 ); }
	| /* empty */				{ $$ = NULL; }

arg_concat
	: arg_concat '^' arg		{ $$ = new ast::Concat( $1, $2 ); }
	| arg

arg
	: word
	| '$' '{' command_list '}'	{ $$ = new ast::Subst( $3 ); }
	| '$' WORD					{ $$ = new ast::Var( $2 ); }
	| '(' arg_list ')'			{ $$ = new ast::List( $2 ); }

word
	: WORD
	| IF						{ $$ = new ast::Word( "if" ); }
	| ELSE						{ $$ = new ast::Word( "else" ); }
	| WHILE						{ $$ = new ast::Word( "while" ); }
	| FOR						{ $$ = new ast::Word( "for" ); }
	| BREAK						{ $$ = new ast::Word( "break" ); }
	| RETURN					{ $$ = new ast::Word( "return" ); }
	| LET						{ $$ = new ast::Word( "let" ); }
	| FUN						{ $$ = new ast::Word( "fun" ); }
