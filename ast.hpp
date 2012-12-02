// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <deque>
#include "glob.hpp"

using namespace std;

namespace ast {


struct Word;
struct Subst;
struct Var;
struct Pair;
struct Concat;
struct Null;
using Expr = Variant<
	Word,
	Subst,
	Var,
	Pair,
	Concat,
	Null
>;

struct VarFix;
struct VarVar;
using LeftExpr = Variant<
	VarFix,
	VarVar
>;

struct If;
struct Command;
struct Fun;
struct FunDel;
struct Let;
struct Fetch;
struct Yield;
struct Return;
struct Break;
struct While;
struct Bg;
struct Sequence;
struct Parallel;
struct RedirFr;
struct RedirTo;
struct Pipe;
struct Defer;
struct None;
using Stmt = Variant<
	If,
	Command,
	Fun,
	FunDel,
	Let,
	Fetch,
	Yield,
	Return,
	Break,
	While,
	Bg,
	Sequence,
	Parallel,
	RedirFr,
	RedirTo,
	Pipe,
	Defer,
	None
>;

struct Word: VariantImpl<Expr, Word> {
	Word( MetaString const& w ):
		word( w ) {}

	MetaString word;
};

struct Subst: VariantImpl<Expr, Subst> {
	Subst( unique_ptr<Stmt>&& b ):
		body( move( b ) ) {}

	unique_ptr<Stmt> body;
};

struct Var: VariantImpl<Expr, Var> {
	Var( string const& n ):
		name( n ) {}

	string name;
};

struct Pair: VariantImpl<Expr, Pair> {
	Pair( unique_ptr<Expr>&& l, unique_ptr<Expr>&& r ):
		lhs( move( l ) ), rhs( move( r ) ) {}

	unique_ptr<Expr> lhs;
	unique_ptr<Expr> rhs;
};

struct Concat: VariantImpl<Expr, Concat> {
	Concat( unique_ptr<Expr>&& l, unique_ptr<Expr>&& r ):
		lhs( move( l ) ), rhs( move( r ) ) {}

	unique_ptr<Expr> lhs;
	unique_ptr<Expr> rhs;
};

struct Null: VariantImpl<Expr, Null> {
	Null() {}
};

struct VarFix: VariantImpl<LeftExpr, VarFix> {
	VarFix( deque<unique_ptr<Expr>>&& v ):
		var( move( v ) ) {}

	deque<unique_ptr<Expr>> var;
};

struct VarVar: VariantImpl<LeftExpr, VarVar> {
	VarVar( deque<unique_ptr<Expr>>&& vL, unique_ptr<Var>&& vM, deque<unique_ptr<Expr>>&& vR ):
		varL( move( vL ) ), varM( move( vM ) ), varR( move( vR ) )  {}

	deque<unique_ptr<Expr>> varL;
	unique_ptr<Var> varM;
	deque<unique_ptr<Expr>> varR;
};

struct If: VariantImpl<Stmt, If> {
	If( unique_ptr<Stmt>&& c, unique_ptr<Stmt>&& t, unique_ptr<Stmt>&& e ):
		cond( move( c ) ), then( move( t ) ), elze( move( e ) ) {}

	unique_ptr<Stmt> cond;
	unique_ptr<Stmt> then;
	unique_ptr<Stmt> elze;
};

struct Command: VariantImpl<Stmt, Command> {
	Command( unique_ptr<Expr>&& a ):
		args( move( a ) ) {}

	unique_ptr<Expr> args;
};

struct Fun: VariantImpl<Stmt, Fun> {
	Fun( unique_ptr<Expr>&& n, unique_ptr<LeftExpr>&& a, unique_ptr<Stmt>&& b ):
		name( move( n ) ), args( move( a ) ), body( move( b ) ) {}

	unique_ptr<Expr> name;
	shared_ptr<LeftExpr> args;
	shared_ptr<Stmt> body;
};

struct FunDel: VariantImpl<Stmt, FunDel> {
	FunDel( unique_ptr<Expr>&& n ):
		name( move( n ) ) {}

	unique_ptr<Expr> name;
};

struct Let: VariantImpl<Stmt, Let> {
	Let( unique_ptr<LeftExpr>&& l, unique_ptr<Expr>&& r ):
		lhs( move( l ) ), rhs( move( r ) ) {}

	unique_ptr<LeftExpr> lhs;
	unique_ptr<Expr> rhs;
};

struct Fetch: VariantImpl<Stmt, Fetch> {
	Fetch( unique_ptr<LeftExpr>&& l ):
		lhs( move( l ) ) {}

	unique_ptr<LeftExpr> lhs;
};

struct Yield: VariantImpl<Stmt, Yield> {
	Yield( unique_ptr<Expr>&& r ):
		rhs( move( r ) ) {}

	unique_ptr<Expr> rhs;
};

struct Return: VariantImpl<Stmt, Return> {
	Return( unique_ptr<Expr>&& r ):
		retv( move( r ) ) {}

	unique_ptr<Expr> retv;
};

struct Break: VariantImpl<Stmt, Break> {
	Break( unique_ptr<Expr>&& r ):
		retv( move( r ) ) {}

	unique_ptr<Expr> retv;
};

struct While: VariantImpl<Stmt, While> {
	While( unique_ptr<Stmt>&& c, unique_ptr<Stmt>&& b, unique_ptr<Stmt>&& e ):
		cond( move( c ) ), body( move( b ) ), elze( move( e ) ) {}

	unique_ptr<Stmt> cond;
	unique_ptr<Stmt> body;
	unique_ptr<Stmt> elze;
};

struct Bg: VariantImpl<Stmt, Bg> {
	Bg( unique_ptr<Stmt>&& b ):
		body( move( b ) ) {}

	shared_ptr<Stmt> body;
};

struct Sequence: VariantImpl<Stmt, Sequence> {
	Sequence( unique_ptr<Stmt>&& l, unique_ptr<Stmt>&& r ):
		lhs( move( l ) ), rhs( move( r ) ) {}

	unique_ptr<Stmt> lhs;
	unique_ptr<Stmt> rhs;
};

struct Parallel: VariantImpl<Stmt, Parallel> {
	Parallel( unique_ptr<Stmt>&& l, unique_ptr<Stmt>&& r ):
		lhs( move( l ) ), rhs( move( r ) ) {}

	unique_ptr<Stmt> lhs;
	unique_ptr<Stmt> rhs;
};

struct RedirFr: VariantImpl<Stmt, RedirFr> {
	RedirFr( unique_ptr<Stmt>&& b, unique_ptr<Expr>&& f ):
		body( move( b ) ), file( move( f ) ) {}

	unique_ptr<Stmt> body;
	unique_ptr<Expr> file;
};

struct RedirTo: VariantImpl<Stmt, RedirTo> {
	RedirTo( unique_ptr<Stmt>&& b, unique_ptr<Expr>&& f ):
		body( move( b ) ), file( move( f ) ) {}

	unique_ptr<Stmt> body;
	unique_ptr<Expr> file;
};

struct Pipe: VariantImpl<Stmt, Pipe> {
	Pipe( unique_ptr<Stmt>&& l, unique_ptr<Stmt>&& r ):
		lhs( move( l ) ), rhs( move( r ) ) {}

	unique_ptr<Stmt> lhs;
	unique_ptr<Stmt> rhs;
};

struct Defer: VariantImpl<Stmt, Defer> {
	Defer( unique_ptr<Expr>&& a ):
		args( move( a ) ) {}

	unique_ptr<Expr> args;
};

struct None: VariantImpl<Stmt, None> {
	None( int r ):
		retv( r ) {}

	int retv;
};


} // namespace ast
