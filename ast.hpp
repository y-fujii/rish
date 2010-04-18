#pragma once

#include <utility>
#include <deque>
#include <string>

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

	protected: Expr( Tag t ): tag( t ) {}
	private: Expr();
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

	protected: Statement( Tag t ): tag( t ) {}
	private: Statement();
};

struct Word: Expr {
	Word( std::string* w ): Expr( tWord ), word( w ) {}
	std::string* word;
};

struct Subst: Expr {
	Subst( Statement* b ): Expr( tSubst ), body( b ) {}
	Statement* body;
};

struct Var: Expr {
	Var( std::string* n ): Expr( tVar ), name( n ) {}
	std::string* name;
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

	protected: VarLhs( Tag t ): tag( t ) {}
	private: VarLhs();
};

struct VarList: VarLhs {
	VarList( std::deque<std::string*>* v ): VarLhs( tVarList ), vars( v ) {}
	std::deque<std::string*>* vars;
};

struct VarStar: VarLhs {
	VarStar( std::deque<std::string*>* h, std::string* s, std::deque<std::string*>* t ):
		VarLhs( tVarStar ), head( h ), star( s ), tail( t ) {}
	std::deque<std::string*>* head;
	std::string* star;
	std::deque<std::string*>* tail;
};

struct If: Statement {
	If( Statement* c, Statement* t, Statement* e ): Statement( tIf ), cond( c ), then( t ), elze( e ) {}
	Statement* cond;
	Statement* then;
	Statement* elze;
};

struct Command: Statement {
	Command( Expr* a ): Statement( tCommand ), args( a ) {}
	Expr* args;
};

struct Fun: Statement {
	Fun( VarLhs* a, Statement* b ): Statement( tFun ), args( a ), body( b ) {}
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
	For( std::string* v, Statement* b, Statement* e ): Statement( tFor ), var( v ), body( b ), elze( e ) {}
	std::string* var;
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
	Bg( Statement* b, std::string* p ): Statement( tBg ), body( b ), pid( p ) {}
	Statement* body;
	std::string* pid;
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
