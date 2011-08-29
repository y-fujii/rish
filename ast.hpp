// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <deque>
#include "glob.hpp"

using namespace std;

namespace ast {


struct Expr: VariantBase<Expr> {
};

struct LeftExpr: VariantBase<LeftExpr> {
};

struct Stmt: VariantBase<Stmt> {
};

struct Word: Variant<Expr, 0> {
	Word( MetaString const& w ): word( w ) {}
	MetaString word;
};

struct Subst: Variant<Expr, 1> {
	Subst( Stmt* b ): body( b ) {}
	Stmt* body;
};

struct Var: Variant<Expr, 2> {
	Var( string const& n ): name( n ) {}
	string name;
};

struct Pair: Variant<Expr, 3> {
	Pair( Expr* l, Expr* r ): lhs( l ), rhs( r ) {}
	Expr* lhs;
	Expr* rhs;
};

struct Null: Variant<Expr, 4> {
	Null() {}
};

struct Concat: Variant<Expr, 5> {
	Concat( Expr* l, Expr* r ): lhs( l ), rhs( r ) {}
	Expr* lhs;
	Expr* rhs;
};

struct VarFix: Variant<LeftExpr, 0> {
	template<class Iter>
	VarFix( Iter bgn, Iter end ): var( bgn, end ) {}
	deque<Expr*> var;
};

struct VarVar: Variant<LeftExpr, 1> {
	template<class Iter>
	VarVar( Iter bgnL, Iter endL, Var* vM, Iter bgnR, Iter endR ):
		varL( bgnL, endL ), varM( vM ), varR( bgnR, endR )  {}
	deque<Expr*> varL;
	Var* varM;
	deque<Expr*> varR;
};

struct If: Variant<Stmt, 0> {
	If( Stmt* c, Stmt* t, Stmt* e ): cond( c ), then( t ), elze( e ) {}
	Stmt* cond;
	Stmt* then;
	Stmt* elze;
};

struct Command: Variant<Stmt, 1> {
	Command( Expr* a ): args( a ) {}
	Expr* args;
};

struct Fun: Variant<Stmt, 2> {
	Fun( Expr* n, LeftExpr* a, Stmt* b ): name( n ), args( a ), body( b ) {}
	Expr* name;
	LeftExpr* args;
	Stmt* body;
};

struct Let: Variant<Stmt, 3> {
	Let( LeftExpr* l, Expr* r ): lhs( l ), rhs( r ) {}
	LeftExpr* lhs;
	Expr* rhs;
};

struct Fetch: Variant<Stmt, 4> {
	Fetch( LeftExpr* l ): lhs( l ) {}
	LeftExpr* lhs;
};

struct Yield: Variant<Stmt, 5> {
	Yield( Expr* r ): rhs( r ) {}
	Expr* rhs;
};

struct Return: Variant<Stmt, 6> {
	Return( Expr* r ): retv( r ) {}
	Expr* retv;
};

struct Break: Variant<Stmt, 7> {
	Break( Expr* r ): retv( r ) {}
	Expr* retv;
};

struct For: Variant<Stmt, 8> {
	For( Expr* v, Stmt* b, Stmt* e ): vars( v ), body( b ), elze( e ) {}
	Expr* vars;
	Stmt* body;
	Stmt* elze;
};

struct While: Variant<Stmt, 9> {
	While( Stmt* c, Stmt* b, Stmt* e ): cond( c ), body( b ), elze( e ) {}
	Stmt* cond;
	Stmt* body;
	Stmt* elze;
};

struct Bg: Variant<Stmt, 10> {
	Bg( Stmt* b ): body( b ) {}
	Stmt* body;
};

struct Sequence: Variant<Stmt, 11> {
	Sequence( Stmt* l, Stmt* r ): lhs( l ), rhs( r ) {}
	Stmt* lhs;
	Stmt* rhs;
};

struct RedirFr: Variant<Stmt, 12> {
	RedirFr( Stmt* b, Expr* f ): body( b ), file( f ) {}
	Stmt* body;
	Expr* file;
};

struct RedirTo: Variant<Stmt, 13> {
	RedirTo( Stmt* b, Expr* f ): body( b ), file( f ) {}
	Stmt* body;
	Expr* file;
};

struct Pipe: Variant<Stmt, 14> {
	Pipe( Stmt* l, Stmt* r ): lhs( l ), rhs( r ) {}
	Stmt* lhs;
	Stmt* rhs;
};

struct Defer: Variant<Stmt, 15> {
	Defer( Expr* a ): args( a ) {}
	Expr* args;
};

struct None: Variant<Stmt, 16> {
	None( int r ): retv( r ) {}
	int retv;
};

} // namespace ast
