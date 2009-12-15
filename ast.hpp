#pragma once

#include <string>

namespace ast {

struct Ast {
	virtual ~Ast() {
	}
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
	List( std::string const& n ): name( n ) {}
	std::string name;
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

struct Cmd: Statement {
	Cmd( Word* c, List* a ): cmd( c ), args( a ) {}
	Word* cmd;
	List* args;
};

struct Fun: Statement {
	Fun() {}
};

}
