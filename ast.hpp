#pragma once

#include <vector>
#include <string>

namespace ast {


struct Arg {
};

struct Statement {
};

struct Word: Arg {
	Word( std::string const& w ): word( w ) {}
	std::string word;
};

struct Subst: Arg {
	Subst( Statement* b ): body( b ) {}
	Statement* body;
};

struct Var: Arg {
	Var( std::string const& n ): name( n ) {}
	std::string name;
};

struct List: Arg {
	List( std::vector<Arg*>* a ): args( a ) {}
	std::vector<Arg*>* args;
};

struct Concat: Arg {
	Concat( Arg* l, Arg* r ): lhs( l ), rhs( r ) {}
	Arg* lhs;
	Arg* rhs;
};

struct VarList {
	VarList( std::vector<std::string>* vs, std::string* vr ): vars( vs ), varr( vr ) {}
	std::vector<std::string>* vars;
	std::string* varr;
};

struct If: Statement {
	If( Statement* c, Statement* t, Statement* e ): cond( c ), then( t ), elze( e ) {}
	Statement* cond;
	Statement* then;
	Statement* elze;
};

struct Command: Statement {
	Command( Arg* c, std::vector<Arg*>* a ): cmd( c ), args( a ) {}
	Arg* cmd;
	std::vector<Arg*>* args;
};

struct Fun: Statement {
	Fun( VarList* a, Statement* b ): args( a ), body( b ) {}
	VarList* args;
	Statement* body;
};

struct Let: Statement {
	Let( VarList* l, std::vector<Arg*>* r ): lhs( l ), rhs( r ) {}
	VarList* lhs;
	std::vector<Arg*>* rhs;
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
	For( std::string* v, Statement* b, Statement* e ): var( v ), body( b ), elze( e ) {}
	std::string* var;
	Statement* body;
	Statement* elze;
};

struct While: Statement {
	While( Statement* c, Statement* b, Statement* e ): cond( c ), body( b ), elze( e ) {}
	Statement* cond;
	Statement* body;
	Statement* elze;
};

struct Not: Statement {
	Not( Statement* b ): body( b ) {}
	Statement* body;
};

struct Or: Statement {
	Or( Statement* l, Statement* r ): lhs( l ), rhs( r ) {}
	Statement* lhs;
	Statement* rhs;
};

struct And: Statement {
	And( Statement* l, Statement* r ): lhs( l ), rhs( r ) {}
	Statement* lhs;
	Statement* rhs;
};

struct Bg: Statement {
	Bg( Statement* b ): body( b ) {}
	Statement* body;
};

struct Seq: Statement {
	Seq( Statement* l, Statement* r ): lhs( l ), rhs( r ) {}
	Statement* lhs;
	Statement* rhs;
};


}
