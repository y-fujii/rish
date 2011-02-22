%{
	#include <exception>
	#include <fcntl.h>
	#include "misc.hpp"
	#include "ast.hpp"
	#include "exception.hpp"

	using namespace std;
	using namespace ast;


	int yylex();

	extern "C" int yyerror( char const* ) {
		throw SyntaxError();
	}

	extern "C" int yywrap() {
		return 1;
	}

	ast::Stmt* parseResult = NULL;
%}

%union {
	MetaString* word;
	std::string* var;
	ast::Expr* expr;
	ast::Stmt* stmt;
}

%type<word> TK_WORD
%type<var> TK_VAR
%type<expr> arg arg_concat args0 args1
%type<stmt> command_seq command_bg command_andor command_not
%type<stmt> command_redir command_pipe command_stat if_ else_

%token TK_AND2 TK_OR2 TK_RDT1 TK_RDT2 TK_RDFR TK_WORD TK_VAR
%token TK_IF TK_ELSE TK_WHILE TK_FOR TK_BREAK TK_RETURN TK_LET TK_FUN
%token TK_WHEN TK_FETCH

%start top

%%

top
	: command_seq						{ ::parseResult = $1; }

command_seq
	: command_seq ';' command_bg		{ $$ = new Sequence( $1, $3 ); }
	| command_seq ';'
	| command_bg
	| 									{ $$ = new None(); }

command_bg
	: '&' command_andor					{ $$ = new Bg( $2 ); }
	| command_andor

command_andor
	: command_andor TK_AND2 command_not	{ $$ = new And( $1, $3 ); }
	| command_andor TK_OR2 command_not	{ $$ = new Or( $1, $3 ); }
	| command_not

command_not
	: '!' command_redir					{ $$ = new Not( $2 ); }
	| command_redir

command_redir
	: command_pipe TK_RDT1 arg			{ $$ = new RedirTo( $1, $3 ); }
	| command_pipe TK_RDT2 arg			{ $$ = new RedirTo( $1, $3 ); }
	| command_pipe

command_pipe
	: command_pipe '|' command_stat		{ $$ = new Pipe( $1, $3 ); }
	| arg_concat TK_RDFR command_stat	{ $$ = new RedirFr( $3, $1 ); }
	| command_stat

command_stat
	: if_
	| TK_WHILE command_andor '{' command_seq '}' else_	{ $$ = new While( $2, $4, $6 ); }
	| TK_FOR args1           '{' command_seq '}' else_	{ $$ = new For( $2, $4, $6 ); }
	| TK_BREAK arg_concat								{ $$ = new Break( $2 ); }
	| TK_RETURN arg_concat								{ $$ = new Return( $2 ); }
	| TK_FETCH args0									{ $$ = new FetchFix( $2 ); }
	| TK_FETCH args0 '@' TK_VAR args0					{ $$ = new LetVar( $2, new Var( *$4 ), $5 ); delete $4; }
	/*
	| TK_LET args0 '=' args0							{ $$ = new LetFix( $2, $4 ); }
	| TK_LET args0 '@' TK_VAR args0 '=' args0			{ $$ = new LetVar( $2, $4, $5, $7 ); }
	*/
	| args0 '=' args0									{ $$ = new LetFix( $1, $3 ); }
	| args0 '@' TK_VAR args0 '=' args0					{ $$ = new LetVar( $1, new Var( *$3 ), $4, $6 ); delete $3; }
	| TK_FUN TK_WORD args0 '{' command_seq '}'			{ $$ = new Fun( *$2, $3, $5 ); delete $2; }
	| '{' command_seq '}'								{ $$ = $2; };
	| args1												{ $$ = new Command( $1 ); }

if_
	: TK_IF command_andor '{' command_seq '}' else_	{ $$ = new If( $2, $4, $6 ); }

else_
	: TK_ELSE '{' command_seq '}'	{ $$ = $3; }
	| TK_ELSE if_					{ $$ = $2; }
	| 								{ $$ = new None(); }

/*
args0
	: args0 arg_concat				{ $$ = new List( $2, $1 ); }
	| 								{ $$ = new Null(); }

args1
	: args1 arg_concat				{ $$ = new List( $2, $1 ); }
	| arg_concat					{ $$ = new List( $1, new Null() ); }
*/
args0
	: arg_concat args0				{ $$ = new List( $1, $2 ); }
	| 								{ $$ = new Null(); }

args1
	: arg_concat args1				{ $$ = new List( $1, $2 ); }
	| arg_concat					{ $$ = new List( $1, new Null() ); }

arg_concat
	: arg_concat '^' arg			{ $$ = new Concat( $1, $3 ); }
	| arg

arg
	: TK_WORD						{ $$ = new Word( *$1 ); delete $1; }
	| '$' '{' command_seq '}'		{ $$ = new Subst( $3 ); }
	| TK_VAR						{ $$ = new Var( *$1 ); delete $1; }
	/*
	| arg '[' arg ']'				{ $$ = new Index( $1, $3 ); }
	| arg '[' arg ':' arg ']'		{ $$ = new Slice( $1, $3, $5 ); }
	*/
	| '(' args0 ')'					{ $$ = $2; }

/*
word_wr
	: TK_WORD
	| TK_IF							{ $$ = new Word( "if" ); }
	| TK_ELSE						{ $$ = new Word( "else" ); }
	| TK_WHILE						{ $$ = new Word( "while" ); }
	| TK_FOR						{ $$ = new Word( "for" ); }
	| TK_BREAK						{ $$ = new Word( "break" ); }
	| TK_RETURN						{ $$ = new Word( "return" ); }
	| TK_LET						{ $$ = new Word( "let" ); }
	| TK_FUN						{ $$ = new Word( "fun" ); }
	| '='							{ $$ = new Word( "=" ); }
	| '*'							{ $$ = new Word( "*" ); }
*/
