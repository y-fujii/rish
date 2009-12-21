%{
	#include "ast.hpp"
%}

%union {
	std::string* string;
	ast::Value* value;
	ast::Statement* statement;
	std::vector< tr1::tuple<Arg*, int> >* redir_to;
	VarList* var_lhs;
	std::vector<Word*> var_list;
	std::vector<Arg*> arg_list;
}

%type<statement> command_list command_bg command_andor command_not
%type<statement> command_redir command_pipe command_stat if else
%type<value> arg arg_concat
%type<redir_to> redir_to
%type<string> WORD
%type<var_lhs> var_lhs
%type<var_list> var_list
%type<arg_list> arg_list

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
	: redir_to RDT1 arg					{ $1->push_back( tr1::make_tuple( $3, O_CREAT ) ); }
	| redir_to RDT2 arg					{ $1->push_back( tr1::make_tuple( $3, O_APPEND ) ); }
	| /* empty */						{ $$ = new std::vector< tr1::tuple<Arg*, int> >(); }

command_pipe
	: command_pipe '|' command_stat		{ $$ = new ast::Pipe( $1, $3 ); }
	| command_stat

command_stat
	: if
	| WHILE command_andor '{' command_list '}' else	{ $$ = new ast::While( $2, $4, $6 ); }
	| FOR WORD            '{' command_list '}' else	{ $$ = new ast::For( $2, $4, $6 ); }
	| BREAK arg										{ $$ = new ast::Break( $2 ); }
	| RETURN arg									{ $$ = new ast::Return( $2 ); }
	| LET var_lhs '=' arg_list						{ $$ = new ast::Let( $2, ast::new List( $4 ) ); }
	| FUN var_lhs '{' command_list '}'				{ $$ = new ast::Fun( $2, $4 ); }
	| '{' command_list '}'							{ $$ = $2 };
	| arg arg_list									{ $2->push_back( $1 ); $$ = new ast::Command( $2 ); }

if
	: IF command_andor '{' command_list '}' else	{ $$ = ast::If( $2, $4, $6 ); }

else
	: ELSE '{' command_list '}'	{ $$ = $3; }
	| ELSE if					{ $$ = $2; }
	| /* empty */				{ $$ = NULL; }

var_lhs
	: var_list '&' WORD var_list	{ $$ = new ast::VarStar( $1, $3, $4 ); }
	| var_list						{ $$ = new ast::VarList( $1 ); }

var_list
	: var_list WORD				{ $1->push_back( $2 ); }
	| /* empty */				{ $$ = new std::vector<Word*>(); }

arg_list
	: arg_list arg_concat		{ $1->push_back( $2 ); }
	| /* empty */				{ $$ = new std::vector<Arg*>(); }

arg_concat
	: arg_concat '^' arg		{ $$ = new ast::Concat( $1, $3 ); }
	| arg

arg
	: WORD						{ $$ = new ast::Word( $1 ); }
	| '$' '{' command_list '}'	{ $$ = new ast::Subst( $3 ); }
	| '$' WORD					{ $$ = new ast::Var( $2 ); }
	/*
	| arg '[' arg ']'			{ $$ = new ast::Index( $1, $2 ); }
	| arg '[' arg arg ']'		{ $$ = new ast::Slice( $1, $2, $3 ); }
	*/
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
