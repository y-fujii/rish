%{
	#include <exception>
	#include <fcntl.h>
	#include "misc.hpp"
	#include "ast.hpp"

	using namespace std;
	using namespace ast;


	int yylex();

	extern "C" int yyerror( char const* ) {
		throw exception();
	}

	extern "C" int yywrap() {
		return 1;
	}

	ast::Statement* parseResult = NULL;
%}

%union {
	MetaString* string;
	ast::Expr* expr;
	ast::Statement* statement;
	std::deque< std::pair<ast::Expr*, int> >* redir_to;
	ast::VarLhs* var_lhs;
	std::deque<MetaString*>* var_list;
}

%type<string> WORD
%type<expr> arg arg_concat arg_list0 arg_list1
%type<statement> command_seq command_bg command_andor command_not
%type<statement> command_redir command_pipe command_stat if_ else_
%type<redir_to> redir_to
%type<var_lhs> var_lhs
%type<var_list> var_list

%token AND2 OR2 RDT1 RDT2 RDFR WORD
%token IF ELSE WHILE FOR BREAK RETURN LET FUN

%start top

%%

top
	: command_seq						{ ::parseResult = $1; }

command_seq
	: command_seq ';' command_bg		{ $$ = new Sequence( $1, $3 ); }
	| command_seq ';'
	| command_bg
	| /* empty */						{ $$ = new None(); }

command_bg
	: command_andor '&' WORD			{ $$ = new Bg( $1, $3 ); }
	| command_andor '&'					{ $$ = new Bg( $1, NULL ); }
	| command_andor

command_andor
	: command_andor AND2 command_not	{ $$ = new And( $1, $3 ); }
	| command_andor OR2 command_not		{ $$ = new Or( $1, $3 ); }
	| command_not

command_not
	: '!' command_redir					{ $$ = new Not( $2 ); }
	| command_redir

command_redir
	: arg RDFR command_pipe redir_to	{ $$ = new Redir( $3, $1, $4 ); }
	| command_pipe redir_to				{ $$ = new Redir( $1, NULL, $2 ); }

redir_to
	: redir_to RDT1 arg					{ $1->push_back( make_pair( $3, O_CREAT ) ); }
	| redir_to RDT2 arg					{ $1->push_back( make_pair( $3, O_APPEND ) ); }
	| /* empty */						{ $$ = new deque< pair<Expr*, int> >(); }

command_pipe
	: command_pipe '|' command_stat		{ $$ = new Pipe( $1, $3 ); }
	| command_stat

command_stat
	: if_
	| WHILE command_andor '{' command_seq '}' else_	{ $$ = new While( $2, $4, $6 ); }
	| FOR WORD            '{' command_seq '}' else_	{ $$ = new For( $2, $4, $6 ); }
	| BREAK arg										{ $$ = new Break( $2 ); }
	| RETURN arg									{ $$ = new Return( $2 ); }
	| LET var_lhs '=' arg_list0						{ $$ = new Let( $2, $4 ); }
	| FUN var_lhs '{' command_seq '}'				{ $$ = new Fun( $2, $4 ); }
	| '{' command_seq '}'							{ $$ = $2; };
	| arg_list1										{ $$ = new Command( $1 ); }

if_
	: IF command_andor '{' command_seq '}' else_	{ $$ = new If( $2, $4, $6 ); }

else_
	: ELSE '{' command_seq '}'	{ $$ = $3; }
	| ELSE if_					{ $$ = $2; }
	| /* empty */				{ $$ = NULL; }

var_lhs
	: var_list '&' WORD var_list	{ $$ = new VarStar( $1, $3, $4 ); }
	| var_list						{ $$ = new VarList( $1 ); }

var_list
	: var_list WORD				{ $1->push_back( $2 ); }
	| /* empty */				{ $$ = new deque<MetaString*>(); }

arg_list0
	: arg_list0 arg_concat		{ $$ = new List( $1, $2 ); }
	| /* empty */				{ $$ = new Null(); }

arg_list1
	: arg_list1 arg_concat		{ $$ = new List( $1, $2 ); }
	| arg_concat				{ $$ = new List( $1, new Null() ); }

arg_concat
	: arg_concat '^' arg		{ $$ = new Concat( $1, $3 ); }
	| arg

arg
	: WORD						{ $$ = new Word( $1 ); }
	| '$' '{' command_seq '}'	{ $$ = new Subst( $3 ); }
	| '$' WORD					{ $$ = new Var( $2 ); }
	/*
	| arg '[' arg ']'			{ $$ = new Index( $1, $2 ); }
	| arg '[' arg arg ']'		{ $$ = new Slice( $1, $2, $3 ); }
	*/
	| '(' arg_list0 ')'			{ $$ = $2; }

/*
word_wr
	: WORD
	| IF						{ $$ = new Word( "if" ); }
	| ELSE						{ $$ = new Word( "else" ); }
	| WHILE						{ $$ = new Word( "while" ); }
	| FOR						{ $$ = new Word( "for" ); }
	| BREAK						{ $$ = new Word( "break" ); }
	| RETURN					{ $$ = new Word( "return" ); }
	| LET						{ $$ = new Word( "let" ); }
	| FUN						{ $$ = new Word( "fun" ); }
	| '='						{ $$ = new Word( "=" ); }
	| '*'						{ $$ = new Word( "*" ); }
*/
