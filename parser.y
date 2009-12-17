%{
	#include "ast.hpp"
	#include "parser.hpp"

	#define YYSTYPE Expr*
%}

%token AND2 OR2 RDT1 RDT2 RDFR EOS
%token IF ELSE WHILE FOR BREAK RETURN LET FUN
%token WORD

%start command_list

%%

command_list
	: command_list ';' command_bg		{ $$ = new ast::Seq( $1, $3 ); }
	| command_bg ';' EOS
	| command_bg EOS

command_bg
	: command_andor '&' arg				{ $$ = new ast::Bg( $1, $3 ); }
	| command_andor '&'					{ $$ = new ast::Bg( $1, NULL ); }
	| command_andor

command_andor
	: command_andor AND2 command_not	{ $$ = new ast::And( $1, $3 ); }
	| command_andor OR2 command_not		{ $$ = new ast::Or( $1, $3 ); }
	| command_not

command_not
	: '!' command_redir					{ $$ = new ast::Not( $2 ); }
	| command_redir

command_redir
	: arg RDFR command_pipe redir_to	{ $$ = new ast::Redir( $3, $1, $4 ); }
	| command_pipe redir_to				{ $$ = new ast::Redir( $1, NULL, $2 ); }

redir_to
	: redir_to RDT1 arg					{ $1->push_back( ast::RedirTo( $3, O_CREAT ) ); }
	| redir_to RDT2 arg					{ $1->push_back( ast::RedirTo( $3, O_APPEND ) ); }
	| /* empty */						{ $$ = new std::vector<ast::RedirTo>(); }

command_pipe
	: command_pipe '|' command_stat		{ $$ = new ast::Pipe( $1, $2 ); }
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
	| arg arg_list									{ $$ = new ast::Cmd( $1, $2 ); }

if
	: IF command_andor '{' command_list '}' else	{ $$ = ast::If( $2, $4, $6 ); }

else
	: ELSE '{' command_list '}'	{ $$ = $3; }
	| ELSE if					{ $$ = $2; }
	| /* empty */				{ $$ = NULL; }

var_list
	: var_list WORD				{ $1->push_back( $2 ); }
	| /* empty */				{ $$ = new std::vector<Word*>(); }

var_star
	: '*' WORD					{ $$ = new ast::Star( $2 ); }
	| /* empty */				{ $$ = NULL; }

arg_list
	: arg_list arg_concat		{ $1->push_back( $2 ); }
	| /* empty */				{ $$ = new std::vector<Arg*>(); }

arg_concat
	: arg_concat '^' arg		{ $$ = new ast::Concat( $1, $2 ); }
	| arg

arg
	: WORD						{ $$ = new ast::Word( $1 ); }
	| '$' '{' command_list '}'	{ $$ = new ast::Subst( $3 ); }
	| '$' WORD					{ $$ = new ast::Var( $2 ); }
	| arg '[' arg ']'			{ $$ = new ast::Index( $1, $2 ); }
	| arg '[' arg arg ']'		{ $$ = new ast::Slice( $1, $2, $3 ); }
	| '(' arg_list ')'			{ $$ = new ast::List( $2 ); }

/*
word_wr
	: WORD
	| IF						{ $$ = new ast::Word( "if" ); }
	| ELSE						{ $$ = new ast::Word( "else" ); }
	| WHILE						{ $$ = new ast::Word( "while" ); }
	| FOR						{ $$ = new ast::Word( "for" ); }
	| BREAK						{ $$ = new ast::Word( "break" ); }
	| RETURN					{ $$ = new ast::Word( "return" ); }
	| LET						{ $$ = new ast::Word( "let" ); }
	| FUN						{ $$ = new ast::Word( "fun" ); }
	| '='						{ $$ = new ast::Word( "=" ); }
	| '*'						{ $$ = new ast::Word( "*" ); }
*/
