// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <deque>
#include "glob.hpp"

using namespace std;

namespace ast {

struct Word;
struct Subst;
struct Var;
struct Pair;
struct Null;
struct Concat;
struct Null;
typedef Variant<
	tmp::Cons<Word,
	tmp::Cons<Subst,
	tmp::Cons<Var,
	tmp::Cons<Pair,
	tmp::Cons<Null,
	tmp::Cons<Concat,
	tmp::Null> > > > > >
> Expr;

struct VarFix;
struct VarVar;
typedef Variant<
	tmp::Cons<VarFix,
	tmp::Cons<VarVar,
	tmp::Null> >
> LeftExpr;

struct If;
struct Command;
struct Fun;
struct Let;
struct Fetch;
struct Yield;
struct Return;
struct Break;
struct While;
struct Bg;
struct Sequence;
struct RedirFr;
struct RedirTo;
struct Pipe;
struct Defer;
struct None;
typedef Variant<
	tmp::Cons<If,
	tmp::Cons<Command,
	tmp::Cons<Fun,
	tmp::Cons<Let,
	tmp::Cons<Fetch,
	tmp::Cons<Yield,
	tmp::Cons<Return,
	tmp::Cons<Break,
	tmp::Cons<While,
	tmp::Cons<Bg,
	tmp::Cons<Sequence,
	tmp::Cons<RedirFr,
	tmp::Cons<RedirTo,
	tmp::Cons<Pipe,
	tmp::Cons<Defer,
	tmp::Cons<None,
	tmp::Null> > > > > > > > > > > > > > > >
> Stmt;

struct Word: VariantImpl<Expr, Word> {
	Word( MetaString const& w ): word( w ) {}
	MetaString word;
};

struct Subst: VariantImpl<Expr, Subst> {
	Subst( Stmt* b ): body( b ) {}
	Stmt* body;
};

struct Var: VariantImpl<Expr, Var> {
	Var( string const& n ): name( n ) {}
	string name;
};

struct Pair: VariantImpl<Expr, Pair> {
	Pair( Expr* l, Expr* r ): lhs( l ), rhs( r ) {}
	Expr* lhs;
	Expr* rhs;
};

struct Null: VariantImpl<Expr, Null> {
	Null() {}
};

struct Concat: VariantImpl<Expr, Concat> {
	Concat( Expr* l, Expr* r ): lhs( l ), rhs( r ) {}
	Expr* lhs;
	Expr* rhs;
};

struct VarFix: VariantImpl<LeftExpr, VarFix> {
	template<class Iter>
	VarFix( Iter bgn, Iter end ): var( bgn, end ) {}
	deque<Expr*> var;
};

struct VarVar: VariantImpl<LeftExpr, VarVar> {
	template<class Iter>
	VarVar( Iter bgnL, Iter endL, Var* vM, Iter bgnR, Iter endR ):
		varL( bgnL, endL ), varM( vM ), varR( bgnR, endR )  {}
	deque<Expr*> varL;
	Var* varM;
	deque<Expr*> varR;
};

struct If: VariantImpl<Stmt, If> {
	If( Stmt* c, Stmt* t, Stmt* e ): cond( c ), then( t ), elze( e ) {}
	Stmt* cond;
	Stmt* then;
	Stmt* elze;
};

struct Command: VariantImpl<Stmt, Command> {
	Command( Expr* a ): args( a ) {}
	Expr* args;
};

struct Fun: VariantImpl<Stmt, Fun> {
	Fun( Expr* n, LeftExpr* a, Stmt* b ): name( n ), args( a ), body( b ) {}
	Expr* name;
	LeftExpr* args;
	Stmt* body;
};

struct Let: VariantImpl<Stmt, Let> {
	Let( LeftExpr* l, Expr* r ): lhs( l ), rhs( r ) {}
	LeftExpr* lhs;
	Expr* rhs;
};

struct Fetch: VariantImpl<Stmt, Fetch> {
	Fetch( LeftExpr* l ): lhs( l ) {}
	LeftExpr* lhs;
};

struct Yield: VariantImpl<Stmt, Yield> {
	Yield( Expr* r ): rhs( r ) {}
	Expr* rhs;
};

struct Return: VariantImpl<Stmt, Return> {
	Return( Expr* r ): retv( r ) {}
	Expr* retv;
};

struct Break: VariantImpl<Stmt, Break> {
	Break( Expr* r ): retv( r ) {}
	Expr* retv;
};

struct While: VariantImpl<Stmt, While> {
	While( Stmt* c, Stmt* b, Stmt* e ): cond( c ), body( b ), elze( e ) {}
	Stmt* cond;
	Stmt* body;
	Stmt* elze;
};

struct Bg: VariantImpl<Stmt, Bg> {
	Bg( Stmt* b ): body( b ) {}
	Stmt* body;
};

struct Sequence: VariantImpl<Stmt, Sequence> {
	Sequence( Stmt* l, Stmt* r ): lhs( l ), rhs( r ) {}
	Stmt* lhs;
	Stmt* rhs;
};

struct RedirFr: VariantImpl<Stmt, RedirFr> {
	RedirFr( Stmt* b, Expr* f ): body( b ), file( f ) {}
	Stmt* body;
	Expr* file;
};

struct RedirTo: VariantImpl<Stmt, RedirTo> {
	RedirTo( Stmt* b, Expr* f ): body( b ), file( f ) {}
	Stmt* body;
	Expr* file;
};

struct Pipe: VariantImpl<Stmt, Pipe> {
	Pipe( Stmt* l, Stmt* r ): lhs( l ), rhs( r ) {}
	Stmt* lhs;
	Stmt* rhs;
};

struct Defer: VariantImpl<Stmt, Defer> {
	Defer( Expr* a ): args( a ) {}
	Expr* args;
};

struct None: VariantImpl<Stmt, None> {
	None( int r ): retv( r ) {}
	int retv;
};

} // namespace ast
