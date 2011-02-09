#pragma once

#include <utility>
#include <deque>
#include "misc.hpp"
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

struct Statement {
	enum Tag {
		tIf,
		tCommand,
		tFun,
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
		tRedir,
		tPipe,
		tNone
	};
	Tag const tag;
	Statement* parent;

	protected:
		Statement( Tag t, Statement* p = NULL ): tag( t ), parent( p ) {}

	private:
		Statement();
};

struct Word: Expr {
	Word( MetaString* w ): Expr( tWord ), word( w ) {}
	MetaString* word;
};

struct Subst: Expr {
	Subst( Statement* b ): Expr( tSubst ), body( b ) {}
	Statement* body;
};

struct Var: Expr {
	Var( MetaString* n ): Expr( tVar ), name( n ) {}
	MetaString* name;
};

struct List: Expr {
	List( Expr* l, Expr* r ): Expr( tList ), lhs( l ), rhs( r ) {}
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

struct VarLhs {
	enum Tag {
		tVarList,
		tVarStar
	};
	Tag const tag;

	protected:
		VarLhs( Tag t ): tag( t ) {}

	private:
		VarLhs();
};

struct VarList: VarLhs {
	VarList( std::deque<Expr*>* v ): VarLhs( tVarList ), vars( v ) {}
	std::deque<Expr*>* vars;
};

struct VarStar: VarLhs {
	VarStar( std::deque<Expr*>* h, MetaString* s, std::deque<Expr*>* t ):
		VarLhs( tVarStar ), head( h ), star( s ), tail( t ) {}
	std::deque<Expr*>* head;
	MetaString* star;
	std::deque<Expr*>* tail;
};

struct If: Statement {
	If( Statement* c, Statement* t, Statement* e ): Statement( tIf ), cond( c ), then( t ), elze( e ) {
		c->parent = t->parent = e->parent = this;
	}
	Statement* cond;
	Statement* then;
	Statement* elze;
};

struct Command: Statement {
	Command( Expr* a ): Statement( tCommand ), args( a ) {}
	Expr* args;
};

struct Fun: Statement {
	Fun( MetaString* n, VarLhs* a, Statement* b ): Statement( tFun ), name( n ), args( a ), body( b ) {}
	MetaString* name;
	VarLhs* args;
	Statement* body;
};

struct Let: Statement {
	Let( VarLhs* l, Expr* r ): Statement( tLet ), lhs( l ), rhs( r ) {}
	VarLhs* lhs;
	Expr* rhs;
};

struct Return: Statement {
	Return( Expr* r ): Statement( tReturn ), retv( r ) {}
	Expr* retv;
};

struct Break: Statement {
	Break( Expr* r ): Statement( tBreak ), retv( r ) {}
	Expr* retv;
};

struct For: Statement {
	For( MetaString* v, Statement* b, Statement* e ): Statement( tFor ), var( v ), body( b ), elze( e ) {
		b->parent = e->parent = this;
	}
	MetaString* var;
	Statement* body;
	Statement* elze;
};

struct While: Statement {
	While( Statement* c, Statement* b, Statement* e ): Statement( tWhile ), cond( c ), body( b ), elze( e ) {}
	Statement* cond;
	Statement* body;
	Statement* elze;
};

struct Not: Statement {
	Not( Statement* b ): Statement( tNot ), body( b ) {}
	Statement* body;
};

struct Or: Statement {
	Or( Statement* l, Statement* r ): Statement( tOr ), lhs( l ), rhs( r ) {}
	Statement* lhs;
	Statement* rhs;
};

struct And: Statement {
	And( Statement* l, Statement* r ): Statement( tAnd ), lhs( l ), rhs( r ) {}
	Statement* lhs;
	Statement* rhs;
};

struct Bg: Statement {
	Bg( Statement* b ): Statement( tBg ), body( b ) {}
	Statement* body;
};

struct Sequence: Statement {
	Sequence( Statement* l, Statement* r ): Statement( tSequence ), lhs( l ), rhs( r ) {}
	Statement* lhs;
	Statement* rhs;
};

struct Redir: Statement {
	Redir( Statement* b, Expr* f, std::deque< std::pair<Expr*, int> >* t ):
		Statement( tRedir ), body( b ), from( f ), to( t ) {}
	Statement* body;
	Expr* from;
	std::deque< std::pair<Expr*, int> >* to;
};

struct Pipe: Statement {
	Pipe( Statement* l, Statement* r ): Statement( tPipe ), lhs( l ), rhs( r ) {}
	Statement* lhs;
	Statement* rhs;
};

struct None: Statement {
	None(): Statement( tNone ) {}
};


} // namespace ast
