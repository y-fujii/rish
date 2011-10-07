// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <deque>
#include "glob.hpp"

using namespace std;

namespace ast {


struct RExpr: VariantBase<RExpr> {
};

struct LExpr: VariantBase<LExpr> {
};

struct Stmt: VariantBase<Stmt> {
};

struct Word: Variant<RExpr, 0> {
	Word( MetaString const& w ): word( w ) {}
	MetaString word;
};

struct Subst: Variant<RExpr, 1> {
	Subst( Stmt* b ): body( b ) {}
	Stmt* body;
};

struct Var: Variant<RExpr, 2> {
	Var( string const& n ): name( n ) {}
	string name;
};

struct Pair: Variant<RExpr, 3> {
	Pair( RExpr* l, RExpr* r ): lhs( l ), rhs( r ) {}
	RExpr* lhs;
	RExpr* rhs;
};

struct Null: Variant<RExpr, 4> {
	Null() {}
};

struct Concat: Variant<RExpr, 5> {
	Concat( RExpr* l, RExpr* r ): lhs( l ), rhs( r ) {}
	RExpr* lhs;
	RExpr* rhs;
};

struct VarFix: Variant<LExpr, 0> {
	template<class Iter>
	VarFix( Iter bgn, Iter end ): var( bgn, end ) {}
	deque<RExpr*> var;
};

struct VarVar: Variant<LExpr, 1> {
	template<class Iter>
	VarVar( Iter bgnL, Iter endL, Var* vM, Iter bgnR, Iter endR ):
		varL( bgnL, endL ), varM( vM ), varR( bgnR, endR )  {}
	deque<RExpr*> varL;
	Var* varM;
	deque<RExpr*> varR;
};

struct If: Variant<Stmt, 0> {
	If( Stmt* c, Stmt* t, Stmt* e ): cond( c ), then( t ), elze( e ) {}
	Stmt* cond;
	Stmt* then;
	Stmt* elze;
};

struct Command: Variant<Stmt, 1> {
	Command( RExpr* a ): args( a ) {}
	RExpr* args;
};

struct Fun: Variant<Stmt, 2> {
	Fun( RExpr* n, LExpr* a, Stmt* b ): name( n ), args( a ), body( b ) {}
	RExpr* name;
	LExpr* args;
	Stmt* body;
};

struct Let: Variant<Stmt, 3> {
	Let( LExpr* l, RExpr* r ): lhs( l ), rhs( r ) {}
	LExpr* lhs;
	RExpr* rhs;
};

struct Fetch: Variant<Stmt, 4> {
	Fetch( LExpr* l ): lhs( l ) {}
	LExpr* lhs;
};

struct Yield: Variant<Stmt, 5> {
	Yield( RExpr* r ): rhs( r ) {}
	RExpr* rhs;
};

struct Return: Variant<Stmt, 6> {
	Return( RExpr* r ): retv( r ) {}
	RExpr* retv;
};

struct Break: Variant<Stmt, 7> {
	Break( RExpr* r ): retv( r ) {}
	RExpr* retv;
};

struct For: Variant<Stmt, 8> {
	For( RExpr* v, Stmt* b, Stmt* e ): vars( v ), body( b ), elze( e ) {}
	RExpr* vars;
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
	RedirFr( Stmt* b, RExpr* f ): body( b ), file( f ) {}
	Stmt* body;
	RExpr* file;
};

struct RedirTo: Variant<Stmt, 13> {
	RedirTo( Stmt* b, RExpr* f ): body( b ), file( f ) {}
	Stmt* body;
	RExpr* file;
};

struct Pipe: Variant<Stmt, 14> {
	Pipe( Stmt* l, Stmt* r ): lhs( l ), rhs( r ) {}
	Stmt* lhs;
	Stmt* rhs;
};

struct Defer: Variant<Stmt, 15> {
	Defer( RExpr* a ): args( a ) {}
	RExpr* args;
};

struct None: Variant<Stmt, 16> {
	None( int r ): retv( r ) {}
	int retv;
};

} // namespace ast
