#pragma once

#include "glob.hpp"

namespace ast {


struct Expr {
	enum Tag {
		tWord,
		tSubst,
		tVar,
		tList,
		tNull,
		tConcat
	};
	Tag const tag;

	protected:
		Expr( Tag t ): tag( t ) {}

	private:
		Expr();
};

struct Stmt {
	enum Tag {
		tIf,
		tCommand,
		tFun,
		tLetFix,
		tLetVar,
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
	Word( MetaString* w ): Expr( tWord ), word( w ) {}
	MetaString* word;
};

struct Subst: Expr {
	Subst( Stmt* b ): Expr( tSubst ), body( b ) {}
	Stmt* body;
};

struct Var: Expr {
	Var( std::string* n ): Expr( tVar ), name( n ) {}
	std::string* name;
};

struct List: Expr {
	List( Expr* l, List* r ): Expr( tList ), lhs( l ), rhs( r ) {}
	Expr* lhs;
	List* rhs;
};

/*
struct Null: Expr {
	Null(): Expr( tNull ) {}
};
*/

struct Concat: Expr {
	Concat( Expr* l, Expr* r ): Expr( tConcat ), lhs( l ), rhs( r ) {}
	Expr* lhs;
	Expr* rhs;
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
	Fun( MetaString* n, Expr* a, Stmt* b ): Stmt( tFun ), name( n ), args( a ), body( b ) {}
	MetaString* name;
	Expr* args;
	Stmt* body;
};

struct LetFix: Stmt {
	LetFix( Expr* l, Expr* r ): Stmt( tLetFix ), lhs( l ), rhs( r ) {}
	Expr* lhs;
	Expr* rhs;
};

struct LetVar: Stmt {
	LetVar( Expr* ll, std::string* lm, Expr* lr, Expr* r ):
		Stmt( tLetVar ), lhsl( ll ), lhsm( lm ), lhsr( lr ), rhs( r ) {}
	Expr* lhsl;
	std::string* lhsm;
	Expr* lhsr;
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
	For( List* v, Stmt* b, Stmt* e ): Stmt( tFor ), vars( v ), body( b ), elze( e ) {}
	List* vars;
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
