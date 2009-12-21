#pragma once

#include <vector>
#include <string>

namespace ast {


struct Expr {
};

struct Statement {
};

struct Word: Expr {
	Word( std::string* w ): word( w ) {}
	std::string* word;
};

struct Subst: Expr {
	Subst( Statement* b ): body( b ) {}
	Statement* body;
};

struct Var: Expr {
	Var( std::string* n ): name( n ) {}
	std::string* name;
};

struct List: Expr {
	List( std::vector<Expr*>* a ): args( a ) {}
	std::vector<Expr*>* args;
};

struct Concat: Expr {
	Concat( Expr* l, Expr* r ): lhs( l ), rhs( r ) {}
	Expr* lhs;
	Expr* rhs;
};

struct VarLhs {
};

struct VarList: VarLhs {
	VarList( std::vector<std::string*>* v ): vars( v ) {}
	std::vector<std::string*>* vars;
};

struct VarStar: VarLhs {
	VarStar( std::vector<std::string*>* h, std::string* s, std::vector<std::string*>* t ):
		head( h ), star( s ), tail( t ) {}
	std::vector<std::string*>* head;
	std::string* star;
	std::vector<std::string*>* tail;
};

struct If: Statement {
	If( Statement* c, Statement* t, Statement* e ): cond( c ), then( t ), elze( e ) {}
	Statement* cond;
	Statement* then;
	Statement* elze;
};

struct Command: Statement {
	Command( Expr* c, std::vector<Expr*>* a ): cmd( c ), args( a ) {}
	Expr* cmd;
	std::vector<Expr*>* args;
};

struct Fun: Statement {
	Fun( VarList* a, Statement* b ): args( a ), body( b ) {}
	VarList* args;
	Statement* body;
};

struct Let: Statement {
	Let( VarList* l, std::vector<Expr*>* r ): lhs( l ), rhs( r ) {}
	VarList* lhs;
	std::vector<Expr*>* rhs;
};

struct Return: Statement {
	Return( Expr* r ): retv( r ) {}
	Expr* retv;
};

struct Break: Statement {
	Break( Expr* r ): retv( r ) {}
	Expr* retv;
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
