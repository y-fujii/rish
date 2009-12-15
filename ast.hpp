#pragma once

#include <string>

namespace ast {

struct Ast {
	virtual ~Ast() {}
};

struct Word: Ast {
	Word( std::string const& w ): word( w ) {}
	std::string word;
};

struct Var: Ast {
	Var( std::string const& n ): name( n ) {}
	std::string name;
};

struct List: Ast {
	List( Word* w, List* n ): word( w ), next( n ) {}
	Word* word;
	List* next;
};

struct Subst: Ast {
	Subst( Seq* s ): seq( s ) {}
	Seq* seq;
};

struct Concat: Ast {
	Concat( Arg* l, Arg* r ): lhs( l ), rhs( r ) {}
	Arg* lhs;
	Arg* rhs;
};

struct Star: Ast {
	Star( std::string n ): name( n ) {}
	std::string name;
};

struct Statement: Ast {
};

struct If: Statement {
	If( c, t, e ): cond( c ), then( t ), elze( e ) {}
	Statement* cond;
	Statement* then;
	Statement* elze;
};

struct Command: Statement {
	Command( Word* c, List* a ): cmd( c ), args( a ) {}
	Word* cmd;
	List* args;
};

struct Fun: Statement {
	Fun( List* as, Word* ar, Command* b ): args( as ), argr( ar ), body( b ) {}
	List* args;
	Word* argr;
	Command* body;
};

struct Let: Statement {
	Let( List* vs, Word* vr, List* v ): vars( vs ), varr( vr ), vals( v ) {}
	List* vars;
	Word* varr;
	List* vals;
};

struct Return: Statement {
	Return( Arg* r ): retv( r ) {}
	Arg* retv;
};

struct Break: Statement {
	Break( Arg* r ): retv( r ) {}
	Arg* retv;
};

struct For: Statement {
	For( Word* v, Command* b, Command* e ): var( v ), body( b ), elze( e ) {}
	Word* var;
	Command* body;
	Command* elze;
};

struct While: Statement {
	While( Command* c, Command* b, Command* e ): cond( c ), body( b ), elze( e ) {}
	Command* cond;
	Command* body;
	Command* elze;
};

struct Not: Statement {
	Not( Command* b ): body( b ) {}
	Command* body;
};

struct Or: Statement {
	Or( Command* l, Command* r ): lhs( l ), rhs( r ) {}
	Command* lhs;
	Command* rhs;
};

struct And: Statement {
	And( Command* l, Command* r ): lhs( l ), rhs( r ) {}
	Command* lhs;
	Command* rhs;
};

struct Bg: Statement {
	And( Command* b ): body( b ) {}
	Command* body;
};

struct Seq: Statement {
	And( Command* l, Command* r ): lhs( l ), rhs( r ) {}
	Command* lhs;
	Command* rhs;
};

}
