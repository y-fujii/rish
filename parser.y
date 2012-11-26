/* (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license */

%{
	#include <exception>
	#include <deque>
	#include <fcntl.h>
	#include "misc.hpp"
	#include "ast.hpp"
	#include "parser.hpp"

	using namespace std;
	using namespace ast;

	unique_ptr<ast::Stmt> parserResult;

	extern "C" int yyerror( char const* ) {
		// XXX: memory leak
		throw SyntaxError( lexerGetLineNo() );
	}
%}

%union {
	MetaString* word;
	std::string* var;
	StupidPtr<ast::Expr> expr;
	StupidPtr<ast::LeftExpr> lexpr;
	StupidPtr<ast::Stmt> stmt;
	std::deque<unique_ptr<ast::Expr>>* exprs;
}

%type<word>  TK_WORD
%type<var>   TK_VAR
%type<expr>  expr_prim expr_concat expr_list
%type<lexpr> lexpr_prim
%type<stmt>  stmt_seq stmt_bg stmt_par stmt_andor stmt_not stmt_redir stmt_pipe
%type<stmt>  stmt_prim if_ else_
%type<exprs> lexpr_list

%token TK_AND2 TK_OR2 TK_RDT1 TK_RDT2 TK_RDFR TK_WORD TK_VAR TK_IF TK_ELSE
%token TK_WHILE TK_BREAK TK_RETURN TK_LET TK_FUN TK_WHEN TK_FETCH TK_YIELD
%token TK_DEFER TK_FOR

%start top

%%

top
	: stmt_seq						{ ::parserResult = $1; }

stmt_seq
	: stmt_seq ';' stmt_bg			{ $$ = new Sequence( $1, $3 ); }
	| stmt_seq ';'
	| stmt_bg
	| 								{ $$ = new None( 0 ); }

stmt_bg
	: '&' stmt_par					{ $$ = new Bg( $2 ); }
	| stmt_par

stmt_par
	: stmt_par '&' stmt_andor		{ $$ = new Parallel( $1, $3 ); }
	| stmt_andor

stmt_andor
	: stmt_andor TK_AND2 stmt_not	{ $$ = new If( $1, $3, make_unique<None>( 1 ) ); }
	| stmt_andor TK_OR2 stmt_not	{ $$ = new If( $1, make_unique<None>( 0 ), $3 ); }
	| stmt_not

stmt_not
	: '!' stmt_redir				{ $$ = new If( $2, make_unique<None>( 1 ), make_unique<None>( 0 ) ); }
	| stmt_redir

stmt_redir
	: stmt_pipe TK_RDT1 expr_concat	{ $$ = new RedirTo( $1, $3 ); }
	| stmt_pipe TK_RDT2 expr_concat	{ $$ = new RedirTo( $1, $3 ); }
	| stmt_pipe

stmt_pipe
	: stmt_pipe '|' stmt_prim		{ $$ = new Pipe( $1, $3 ); }
	| expr_concat TK_RDFR stmt_prim	{ $$ = new RedirFr( $3, $1 ); }
	| stmt_prim

stmt_prim
	: if_
	| TK_WHILE stmt_andor '{' stmt_seq '}' else_		{ $$ = new While( $2, $4, $6 ); }
	| TK_FOR lexpr_prim '{' stmt_seq '}' else_			{ $$ = nullptr; }
	/*
	| TK_FOR lexpr_prim TK_IF stmt_andor				{ $$ = nullptr; }
	*/
	| TK_FOR lexpr_prim TK_IF stmt_andor '{' stmt_seq '}' else_ { $$ = nullptr; }
	| TK_BREAK expr_concat								{ $$ = new Break( $2 ); }
	| TK_RETURN expr_concat								{ $$ = new Return( $2 ); }
	| TK_LET lexpr_prim '=' expr_list					{ $$ = new Let( $2, $4 ); }
	| TK_FETCH lexpr_prim								{ $$ = new Fetch( $2 ); }
	| TK_YIELD expr_list								{ $$ = new Yield( $2 ); }
	| TK_DEFER expr_list								{ $$ = new Defer( $2 ); }
	| TK_FUN expr_concat lexpr_prim '{' stmt_seq '}'	{ $$ = new Fun( $2, $3, $5 ); }
	| TK_FUN expr_concat '!'							{ $$ = new FunDel( $2 ); }
	| '{' stmt_seq '}'									{ $$ = $2; };
	| expr_concat expr_list								{ $$ = new Command( make_unique<Pair>( $1, $2 ) ); }

if_
	: TK_IF stmt_andor '{' stmt_seq '}' else_			{ $$ = new If( $2, $4, $6 ); }

else_
	: TK_ELSE '{' stmt_seq '}'			{ $$ = $3; }
	| TK_ELSE if_						{ $$ = $2; }
	| 									{ $$ = new None( 0 ); }

lexpr_prim
	: lexpr_list						{ $$ = new VarFix( move( *$1 ) ); delete $1; }
	| lexpr_list '(' TK_VAR ')' lexpr_list	{ $$ = new VarVar( move( *$1 ), make_unique<Var>( *$3 ), move( *$5 ) ); delete $1; delete $3; delete $5; }

lexpr_list
	: lexpr_list TK_VAR					{ $1->push_back( make_unique<Var>( *$2 ) ); delete $2; $$ = $1; }
	| lexpr_list TK_WORD				{ $1->push_back( make_unique<Word>( *$2 ) ); delete $2; $$ = $1; }
	|									{ $$ = new deque<unique_ptr<Expr>>(); }

expr_list
	: expr_concat expr_list				{ $$ = new Pair( $1, $2 ); }
	| 									{ $$ = new Null(); }

expr_concat
	: expr_concat '^' expr_prim			{ $$ = new Concat( $1, $3 ); }
	| expr_prim

expr_prim
	: TK_WORD							{ $$ = new Word( *$1 ); delete $1; }
	| TK_VAR							{ $$ = new Var( *$1 ); delete $1; }
	| '(' stmt_seq ')'					{ $$ = new Subst( $2 ); }
	| '$' '(' TK_WORD expr_concat expr_concat ')'	{ $$ = new Slice( make_unique<Var>( string( $3->begin(), $3->end() ) ), $4, $5 ); delete $3; }
	| '$' '(' TK_WORD expr_concat ')'				{ $$ = new Index( make_unique<Var>( string( $3->begin(), $3->end() ) ), $4 ); delete $3; }
	/*
	| TK_FUN lexpr_prim '{' stmt_seq '}'			{ $$ = NULL; }
	*/
