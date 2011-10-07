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

	ast::Stmt* parserResult;

	extern "C" int yyerror( char const* ) {
		// XXX: memory leaked on yyval.word, yyval.var
		throw SyntaxError( lexerGetLineNo() );
	}
%}

%union {
	MetaString* word;
	std::string* var;
	ast::RExpr* rexpr;
	ast::LExpr* lexpr;
	ast::Stmt* stmt;
	std::deque<ast::RExpr*>* exprs;
}

%type<word> TK_WORD
%type<var> TK_VAR
%type<rexpr> rexpr_prim rexpr_concat rexpr_list
%type<lexpr> lexpr_when lexpr_prim
%type<stmt> stmt_seq stmt_bg stmt_andor stmt_not stmt_redir stmt_pipe stmt_prim
%type<stmt> if_ else_
%type<exprs> lexpr_list

%token TK_AND2 TK_OR2 TK_RDT1 TK_RDT2 TK_RDFR TK_WORD TK_VAR TK_IF TK_ELSE
%token TK_WHILE TK_FOR TK_BREAK TK_RETURN TK_LET TK_FUN TK_WHEN TK_FETCH
%token TK_YIELD TK_DEFER

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
	: '&' stmt_andor				{ $$ = new Bg( $2 ); }
	| stmt_andor

stmt_andor
	: stmt_andor TK_AND2 stmt_not	{ $$ = new If( $1, $3, new None( 1 ) ); }
	| stmt_andor TK_OR2 stmt_not	{ $$ = new If( $1, new None( 0 ), $3 ); }
	| stmt_not

stmt_not
	: '!' stmt_redir				{ $$ = new If( $2, new None( 1 ), new None( 0 ) ); }
	| stmt_redir

stmt_redir
	: stmt_pipe TK_RDT1 rexpr_concat	{ $$ = new RedirTo( $1, $3 ); }
	| stmt_pipe TK_RDT2 rexpr_concat	{ $$ = new RedirTo( $1, $3 ); }
	| stmt_pipe

stmt_pipe
	: stmt_pipe '|' stmt_prim		{ $$ = new Pipe( $1, $3 ); }
	| rexpr_concat TK_RDFR stmt_prim	{ $$ = new RedirFr( $3, $1 ); }
	| stmt_prim

stmt_prim
	: if_
	| TK_WHILE stmt_andor '{' stmt_seq '}' else_	{ $$ = new While( $2, $4, $6 ); }
	/*
	| TK_FOR lexpr_when   '{' stmt_seq '}' else_	{ $$ = new For( $2, $4, $6 ); }
	*/
	| TK_BREAK rexpr_concat							{ $$ = new Break( $2 ); }
	| TK_RETURN rexpr_concat						{ $$ = new Return( $2 ); }
	| TK_LET lexpr_when '=' rexpr_list				{ $$ = new Let( $2, $4 ); }
	| TK_FETCH lexpr_when							{ $$ = new Fetch( $2 ); }
	| TK_YIELD rexpr_list							{ $$ = new Yield( $2 ); }
	| TK_DEFER rexpr_list							{ $$ = new Defer( $2 ); }
	| TK_FUN rexpr_concat lexpr_when '{' stmt_seq '}'	{ $$ = new Fun( $2, $3, $5 ); }
	| '{' stmt_seq '}'								{ $$ = $2; };
	| rexpr_concat rexpr_list						{ $$ = new Command( new Pair( $1, $2 ) ); }

if_
	: TK_IF stmt_andor '{' stmt_seq '}' else_		{ $$ = new If( $2, $4, $6 ); }

else_
	: TK_ELSE '{' stmt_seq '}'			{ $$ = $3; }
	| TK_ELSE if_						{ $$ = $2; }
	| 									{ $$ = new None( 0 ); }

lexpr_when
	/*
	: lexpr_prim TK_WHEN stmt_prim
	*/
	: lexpr_prim

lexpr_prim
	: lexpr_list						{ $$ = new VarFix( $1->begin(), $1->end() ); delete $1; }
	| lexpr_list '(' TK_VAR ')' lexpr_list	{ $$ = new VarVar( $1->begin(), $1->end(), new Var( *$3 ), $5->begin(), $5->end() ); delete $1; delete $3; delete $5; }

lexpr_list
	: lexpr_list TK_VAR					{ $1->push_back( new Var( *$2 ) ); delete $2; $$ = $1; }
	| lexpr_list TK_WORD				{ $1->push_back( new Word( *$2 ) ); delete $2; $$ = $1; }
	|									{ $$ = new deque<RExpr*>(); }

rexpr_list
	: rexpr_concat rexpr_list			{ $$ = new Pair( $1, $2 ); }
	| 									{ $$ = new Null(); }

rexpr_concat
	: rexpr_concat '^' rexpr_prim		{ $$ = new Concat( $1, $3 ); }
	| rexpr_prim

rexpr_prim
	: TK_WORD							{ $$ = new Word( *$1 ); delete $1; }
	| TK_VAR							{ $$ = new Var( *$1 ); delete $1; }
	| '(' stmt_seq ')'					{ $$ = new Subst( $2 ); }
	/*
	| '$' '(' TK_WORD rexpr ')'			{ $$ = new Slice( $3, $4, $4 ); }
	| '$' '(' TK_WORD rexpr rexpr ')'	{ $$ = new Slice( $3, $4, $5 ); }
	| TK_FUN lexpr_when '{' stmt_seq '}' { $$ = NULL; }
	*/
