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
	StupidPtr<ast::Word>     word;
	StupidPtr<ast::Var>      var;
	StupidPtr<ast::Expr>     expr;
	StupidPtr<ast::LeftExpr> lexpr;
	StupidPtr<ast::Stmt>     stmt;
	std::deque<unique_ptr<ast::Expr>>* exprs;
}

%type<word>  TK_WORD symbols
%type<var>   TK_VAR TK_INDEX TK_SIZE
%type<expr>  expr_prim expr_concat expr_pair
%type<expr>  arith_prim arith_concat arith_pair arith_pos arith_mul arith_div
%type<expr>  arith_add arith_bool
%type<lexpr> lexpr_prim
%type<stmt>  stmt_seq stmt_empty stmt_bg stmt_par stmt_andor stmt_not
%type<stmt>  stmt_redir stmt_pipe stmt_prim if_ else_
%type<exprs> expr_list lexpr_list

%token TK_AND2 TK_OR2 TK_RDT1 TK_RDT2 TK_RDFR TK_WORD TK_VAR TK_IF TK_ELSE
%token TK_WHILE TK_BREAK TK_RETURN TK_LET TK_FUN TK_WHEN TK_FETCH TK_YIELD
%token TK_DEFER TK_FOR TK_ZIP TK_CHDIR TK_ARROW TK_EQ TK_NE TK_LE TK_GE
%token TK_INDEX TK_SIZE TK_LETBANG TK_FETCHBANG

%start top

%%

top
	: stmt_seq						{ ::parserResult = $1; }

/* use right recursion for tail-call optimized evaluator */
stmt_seq
	: stmt_empty ';' stmt_seq		{ $$ = new Sequence( $1, $3 ); }
	| stmt_empty

stmt_empty
	:								{ $$ = new None( 0 ); }
	| stmt_bg

stmt_bg
	: '&' stmt_par					{ $$ = new Bg( $2 ); }
	| stmt_par

stmt_par
	: stmt_par '&' stmt_andor		{ $$ = new Parallel( $1, $3 ); }
	| stmt_andor

/* use right recursion for tail-call optimized evaluator */
stmt_andor
	: stmt_not TK_AND2 stmt_andor	{ $$ = new If( $1, $3, make_unique<None>( 1 ) ); }
	| stmt_not TK_OR2  stmt_andor	{ $$ = new If( $1, make_unique<None>( 0 ), $3 ); }
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
	| expr_pair TK_RDFR  stmt_prim	{ $$ = new RedirFr( $3, $1 ); }
	| expr_pair TK_ARROW stmt_prim	{ $$ = new Pipe( make_unique<Yield>( $1 ), $3 ); }
	| stmt_prim

stmt_prim
	: if_
	| TK_WHILE stmt_andor '{' stmt_seq '}' else_		{ $$ = new While( $2, $4, $6 ); }
	| TK_FOR lexpr_prim '{' stmt_seq '}' else_			{ $$ = nullptr; }
	| TK_FOR lexpr_prim TK_IF stmt_andor TK_YIELD expr_pair { $$ = nullptr; }
	| TK_FOR lexpr_prim TK_IF stmt_andor '{' stmt_seq '}' else_ { $$ = nullptr; }
	| TK_BREAK expr_pair								{ $$ = new Break( $2 ); }
	| TK_RETURN expr_pair								{ $$ = new Return( $2 ); }
	| TK_LET lexpr_prim '=' arith_bool					{ $$ = new Let( $2, $4 ); }
	| TK_LET lexpr_prim '=' arith_add					{ $$ = new Let( $2, $4 ); }
	| TK_FETCH lexpr_prim								{ $$ = new Fetch( $2 ); }
	| TK_YIELD expr_pair								{ $$ = new Yield( $2 ); }
	| TK_ZIP expr_list									{ $$ = new Zip( move( *$2 ) ); delete $2; }
	| TK_DEFER expr_pair								{ $$ = new Defer( $2 ); }
	| TK_CHDIR expr_pair								{ $$ = new ChDir( $2 ); }
	| TK_FUN expr_concat lexpr_prim '{' stmt_seq '}'	{ $$ = new Fun( $2, $3, $5 ); }
	| TK_FUN expr_concat '!'							{ $$ = new FunDel( $2 ); }
	| '{' stmt_seq '}'									{ $$ = $2; };
	| expr_concat expr_pair								{ $$ = new Command( make_unique<Pair>( $1, $2 ) ); }

if_
	: TK_IF stmt_andor '{' stmt_seq '}' else_			{ $$ = new If( $2, $4, $6 ); }

else_
	: TK_ELSE '{' stmt_seq '}'			{ $$ = $3; }
	| TK_ELSE if_						{ $$ = $2; }
	| 									{ $$ = new None( 0 ); }

lexpr_prim
	: lexpr_list						{ $$ = new LeftFix( move( *$1 ) ); delete $1; }
	| lexpr_list '(' TK_VAR ')' lexpr_list	{ $$ = new LeftVar( move( *$1 ), $3, move( *$5 ) ); delete $1; delete $5; }

lexpr_list
	: lexpr_list TK_VAR					{ $1->push_back( $2 ); $$ = $1; }
	| lexpr_list TK_WORD				{ $1->push_back( $2 ); $$ = $1; }
	|									{ $$ = new deque<unique_ptr<Expr>>(); }

expr_list
	: expr_list expr_concat				{ $1->push_back( $2 ); $$ = $1; }
	| 									{ $$ = new deque<unique_ptr<Expr>>(); }

/* use right recursion for tail-call optimized evaluator */
expr_pair
	: expr_concat expr_pair				{ $$ = new Pair( $1, $2 ); }
	| 									{ $$ = new Null(); }

expr_concat
	: expr_concat '^' expr_prim			{ $$ = new Concat( $1, $3 ); }
	| expr_prim

expr_prim
	: symbols							{ $$ = $1; }
	| arith_prim
	| '(' arith_bool ')'				{ $$ = $2; }

arith_bool
	: arith_add TK_EQ arith_add			{ $$ = new BinOp( BinOp::eq, $1, $3 ); }
	| arith_add TK_NE arith_add			{ $$ = new BinOp( BinOp::ne, $1, $3 ); }
	| arith_add TK_LE arith_add			{ $$ = new BinOp( BinOp::le, $1, $3 ); }
	| arith_add TK_GE arith_add			{ $$ = new BinOp( BinOp::ge, $1, $3 ); }
	| arith_add '<'   arith_add			{ $$ = new BinOp( BinOp::lt, $1, $3 ); }
	| arith_add '>'   arith_add			{ $$ = new BinOp( BinOp::gt, $1, $3 ); }

arith_add
	: arith_add '+' arith_div			{ $$ = new BinOp( BinOp::add, $1, $3 ); }
	| arith_add '-' arith_div			{ $$ = new BinOp( BinOp::sub, $1, $3 ); }
	| arith_div

arith_div
	: arith_div '/' arith_mul			{ $$ = new BinOp( BinOp::div, $1, $3 ); }
	| arith_div '%' arith_mul			{ $$ = new BinOp( BinOp::mod, $1, $3 ); }
	| arith_mul

arith_mul
	: arith_mul '*' arith_pos			{ $$ = new BinOp( BinOp::mul, $1, $3 ); }
	| arith_pos

arith_pos
	: '+' arith_pair					{ $$ = new UniOp( UniOp::pos, $2 ); }
	| '-' arith_pair					{ $$ = new UniOp( UniOp::neg, $2 ); }
	| arith_pair

arith_pair
	: arith_concat arith_pair			{ $$ = new Pair( $1, $2 ); }
	| arith_concat

arith_concat
	: arith_concat '^' arith_prim		{ $$ = new Concat( $1, $3 ); }
	| arith_prim

arith_prim
	: TK_WORD							{ $$ = $1; }
	| '~'								{ $$ = new Home(); }
	| TK_VAR							{ $$ = $1; }
	| TK_SIZE							{ $$ = new Size( $1 ); }
	| TK_INDEX arith_add ')'			{ $$ = new Index( $1, $2 ); }
	| TK_INDEX arith_add ':' arith_add ')'	{ $$ = new Slice( $1, $2, $4 ); }
	| '(' ')'							{ $$ = new Null(); }
	| '(' arith_add ')'					{ $$ = $2; }
	| '[' stmt_seq ']'					{ $$ = new Subst( $2 ); }

symbols
	: '+'								{ $$ = new Word( basic_string<uint16_t>{ '+' } ); }
	| '-'								{ $$ = new Word( basic_string<uint16_t>{ '-' } ); }
	| '*'								{ $$ = new Word( basic_string<uint16_t>{ star } ); }
	| '/'								{ $$ = new Word( basic_string<uint16_t>{ '/' } ); }
	| '%'								{ $$ = new Word( basic_string<uint16_t>{ '%' } ); }
	| '<'								{ $$ = new Word( basic_string<uint16_t>{ '<' } ); }
	| '>'								{ $$ = new Word( basic_string<uint16_t>{ '>' } ); }
	| ':'								{ $$ = new Word( basic_string<uint16_t>{ ':' } ); }
	| TK_EQ								{ $$ = new Word( basic_string<uint16_t>{ '=', '=' } ); }
	| TK_NE								{ $$ = new Word( basic_string<uint16_t>{ '!', '=' } ); }
	| TK_LE								{ $$ = new Word( basic_string<uint16_t>{ '<', '=' } ); }
	| TK_GE								{ $$ = new Word( basic_string<uint16_t>{ '>', '=' } ); }
