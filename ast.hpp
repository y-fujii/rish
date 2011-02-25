// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <deque>
#include "glob.hpp"

using namespace std;

namespace ast {


struct Expr {
	enum Tag {
		tWord,
		tSubst,
		tVar,
		tPair,
		tNull,
		tConcat
	};
	Tag const tag;

	protected:
		Expr( Tag t ): tag( t ) {}

	private:
		Expr();
};

struct LeftExpr {
	enum Tag {
		tVarFix,
		tVarVar
	};
	Tag const tag;

	protected:
		LeftExpr( Tag t ): tag( t ) {}
	
	private:
		LeftExpr();
};

struct Stmt {
	enum Tag {
		tIf,
		tCommand,
		tFun,
		tFetch,
		tLet,
		tReturn,
		tBreak,
		tFor,
		tWhile,
		tNot,
		tOr,
		tAnd,
		tBg,
		tSequence,
		tRedirFr,
		tRedirTo,
		tPipe,
		tNone
	};
	Tag const tag;

	protected:
		Stmt( Tag t ): tag( t ) {}

	private:
		Stmt();
};

struct Word: Expr {
	Word( MetaString const& w ): Expr( tWord ), word( w ) {}
	MetaString word;
};

struct Subst: Expr {
	Subst( Stmt* b ): Expr( tSubst ), body( b ) {}
	Stmt* body;
};

struct Var: Expr {
	Var( string const& n ): Expr( tVar ), name( n ) {}
	string name;
};

struct Pair: Expr {
	Pair( Expr* l, Expr* r ): Expr( tPair ), lhs( l ), rhs( r ) {}
	Expr* lhs;
	Expr* rhs;
};

struct Null: Expr {
	Null(): Expr( tNull ) {}
};

struct Concat: Expr {
	Concat( Expr* l, Expr* r ): Expr( tConcat ), lhs( l ), rhs( r ) {}
	Expr* lhs;
	Expr* rhs;
};

struct VarFix: LeftExpr {
	template<class Iter>
	VarFix( Iter bgn, Iter end ): LeftExpr( tVarFix ), var( bgn, end ) {}
	deque<Expr*> var;
};

struct VarVar: LeftExpr {
	template<class Iter>
	VarVar( Iter bgnL, Iter endL, Var* vM, Iter bgnR, Iter endR ):
		LeftExpr( tVarVar ), varL( bgnL, endL ), varM( vM ), varR( bgnR, endR )  {}
	deque<Expr*> varL;
	Var* varM;
	deque<Expr*> varR;
};

struct If: Stmt {
	If( Stmt* c, Stmt* t, Stmt* e ): Stmt( tIf ), cond( c ), then( t ), elze( e ) {}
	Stmt* cond;
	Stmt* then;
	Stmt* elze;
};

struct Command: Stmt {
	Command( Expr* a ): Stmt( tCommand ), args( a ) {}
	Expr* args;
};

struct Fun: Stmt {
	Fun( string const& n, Expr* a, Stmt* b ): Stmt( tFun ), name( n ), args( a ), body( b ) {}
	string name;
	Expr* args;
	Stmt* body;
};

struct Fetch: Stmt {
	Fetch( LeftExpr* l ): Stmt( tFetch ), lhs( l ) {}
	LeftExpr* lhs;
};

struct Let: Stmt {
	Let( LeftExpr* l, Expr* r ): Stmt( tLet ), lhs( l ), rhs( r ) {}
	LeftExpr* lhs;
	Expr* rhs;
};

struct Return: Stmt {
	Return( Expr* r ): Stmt( tReturn ), retv( r ) {}
	Expr* retv;
};

struct Break: Stmt {
	Break( Expr* r ): Stmt( tBreak ), retv( r ) {}
	Expr* retv;
};

struct For: Stmt {
	For( Expr* v, Stmt* b, Stmt* e ): Stmt( tFor ), vars( v ), body( b ), elze( e ) {}
	Expr* vars;
	Stmt* body;
	Stmt* elze;
};

struct While: Stmt {
	While( Stmt* c, Stmt* b, Stmt* e ): Stmt( tWhile ), cond( c ), body( b ), elze( e ) {}
	Stmt* cond;
	Stmt* body;
	Stmt* elze;
};

struct Not: Stmt {
	Not( Stmt* b ): Stmt( tNot ), body( b ) {}
	Stmt* body;
};

struct Or: Stmt {
	Or( Stmt* l, Stmt* r ): Stmt( tOr ), lhs( l ), rhs( r ) {}
	Stmt* lhs;
	Stmt* rhs;
};

struct And: Stmt {
	And( Stmt* l, Stmt* r ): Stmt( tAnd ), lhs( l ), rhs( r ) {}
	Stmt* lhs;
	Stmt* rhs;
};

struct Bg: Stmt {
	Bg( Stmt* b ): Stmt( tBg ), body( b ) {}
	Stmt* body;
};

struct Sequence: Stmt {
	Sequence( Stmt* l, Stmt* r ): Stmt( tSequence ), lhs( l ), rhs( r ) {}
	Stmt* lhs;
	Stmt* rhs;
};

struct RedirFr: Stmt {
	RedirFr( Stmt* b, Expr* f ): Stmt( tRedirFr ), body( b ), file( f ) {}
	Stmt* body;
	Expr* file;
};

struct RedirTo: Stmt {
	RedirTo( Stmt* b, Expr* f ): Stmt( tRedirTo ), body( b ), file( f ) {}
	Stmt* body;
	Expr* file;
};

struct Pipe: Stmt {
	Pipe( Stmt* l, Stmt* r ): Stmt( tPipe ), lhs( l ), rhs( r ) {}
	Stmt* lhs;
	Stmt* rhs;
};

struct None: Stmt {
	None(): Stmt( tNone ) {}
};


} // namespace ast
