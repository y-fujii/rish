// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <deque>
#include "glob.hpp"

using namespace std;

namespace ast {


using Expr = Variant<
	struct Word,
	struct Home,
	struct Subst,
	struct Var,
	struct Pair,
	struct Concat,
	struct BinOp,
	struct UniOp,
	struct Size,
	struct Index,
	struct Slice,
	struct Null
>;

using LeftExpr = Variant<
	struct LeftFix,
	struct LeftVar
>;

using Stmt = Variant<
	struct If,
	struct Command,
	struct Fun,
	struct FunDel,
	struct Let,
	struct Fetch,
	struct Yield,
	struct Return,
	struct Break,
	struct While,
	struct Bg,
	struct Sequence,
	struct Parallel,
	struct RedirFr,
	struct RedirTo,
	struct Pipe,
	struct Zip,
	struct Defer,
	struct ChDir,
	struct None
>;

struct Word: VariantImpl<Expr, Word> {
	Word( MetaString const& w ):
		word( w ) {}

	MetaString word;
};

struct Home: VariantImpl<Expr, Home> {
};

struct Subst: VariantImpl<Expr, Subst> {
	Subst( unique_ptr<Stmt>&& b ):
		body( move( b ) ) {}

	unique_ptr<Stmt> body;
};

struct Var: VariantImpl<Expr, Var> {
	Var( string const& n, size_t l ):
		index( -1 ), depth( -1 ), name( n ), line( l ) {}

	int index;
	int depth;
	string name;
	size_t line;
};

struct Pair: VariantImpl<Expr, Pair> {
	Pair( unique_ptr<Expr>&& l, unique_ptr<Expr>&& r ):
		lhs( move( l ) ), rhs( move( r ) ) {}

	unique_ptr<Expr> lhs;
	unique_ptr<Expr> rhs;
};

struct Concat: VariantImpl<Expr, Concat> {
	Concat( unique_ptr<Expr>&& l, unique_ptr<Expr>&& r ):
		lhs( move( l ) ), rhs( move( r ) ) {}

	unique_ptr<Expr> lhs;
	unique_ptr<Expr> rhs;
};

struct BinOp: VariantImpl<Expr, BinOp> {
	enum Operator {
		add, sub, mul, div, mod,
		eq, ne, le, ge, lt, gt,
	};

	BinOp( Operator o, unique_ptr<Expr>&& l, unique_ptr<Expr>&& r ):
		op( o ), lhs( move( l ) ), rhs( move( r ) ) {}

	Operator op;
	unique_ptr<Expr> lhs;
	unique_ptr<Expr> rhs;
};

struct UniOp: VariantImpl<Expr, UniOp> {
	enum Operator {
		pos, neg,
	};

	UniOp( Operator o, unique_ptr<Expr>&& l ):
		op( o ), lhs( move( l ) ) {}

	Operator op;
	unique_ptr<Expr> lhs;
};

struct Size: VariantImpl<Expr, Size> {
	Size( unique_ptr<Var>&& v ):
		var( move( v ) ) {}

	unique_ptr<Var> var;
};

struct Index: VariantImpl<Expr, Index> {
	Index( unique_ptr<Var>&& v, unique_ptr<Expr>&& i ):
		var( move( v ) ), idx( move( i ) ) {}

	unique_ptr<Var> var;
	unique_ptr<Expr> idx;
};

struct Slice: VariantImpl<Expr, Slice> {
	Slice( unique_ptr<Var>&& v, unique_ptr<Expr>&& b, unique_ptr<Expr>&& e ):
		var( move( v ) ), bgn( move( b ) ), end( move( e ) ) {}

	unique_ptr<Var> var;
	unique_ptr<Expr> bgn;
	unique_ptr<Expr> end;
};

struct Null: VariantImpl<Expr, Null> {
	Null() {}
};

struct LeftFix: VariantImpl<LeftExpr, LeftFix> {
	LeftFix( deque<unique_ptr<Expr>>&& v ):
		var( move( v ) ) {}

	deque<unique_ptr<Expr>> var;
};

struct LeftVar: VariantImpl<LeftExpr, LeftVar> {
	LeftVar( deque<unique_ptr<Expr>>&& vL, unique_ptr<Var>&& vM, deque<unique_ptr<Expr>>&& vR ):
		varL( move( vL ) ), varM( move( vM ) ), varR( move( vR ) )  {}

	deque<unique_ptr<Expr>> varL;
	unique_ptr<Var> varM;
	deque<unique_ptr<Expr>> varR;
};

struct If: VariantImpl<Stmt, If> {
	If( unique_ptr<Stmt>&& c, unique_ptr<Stmt>&& t, unique_ptr<Stmt>&& e ):
		cond( move( c ) ), then( move( t ) ), elze( move( e ) ) {}

	unique_ptr<Stmt> cond;
	unique_ptr<Stmt> then;
	unique_ptr<Stmt> elze;
};

struct Command: VariantImpl<Stmt, Command> {
	Command( unique_ptr<Expr>&& a ):
		args( move( a ) ) {}

	unique_ptr<Expr> args;
};

struct Fun: VariantImpl<Stmt, Fun> {
	Fun( unique_ptr<Expr>&& n, unique_ptr<LeftExpr>&& a, unique_ptr<Stmt>&& b ):
		name( move( n ) ), args( move( a ) ), body( move( b ) ) {}

	unique_ptr<Expr> name;
	shared_ptr<LeftExpr> args;
	shared_ptr<Stmt> body;
	int nVar;
};

struct FunDel: VariantImpl<Stmt, FunDel> {
	FunDel( unique_ptr<Expr>&& n ):
		name( move( n ) ) {}

	unique_ptr<Expr> name;
};

struct Let: VariantImpl<Stmt, Let> {
	Let( unique_ptr<LeftExpr>&& l, unique_ptr<Expr>&& r ):
		lhs( move( l ) ), rhs( move( r ) ) {}

	unique_ptr<LeftExpr> lhs;
	unique_ptr<Expr> rhs;
};

struct Fetch: VariantImpl<Stmt, Fetch> {
	Fetch( unique_ptr<LeftExpr>&& l ):
		lhs( move( l ) ) {}

	unique_ptr<LeftExpr> lhs;
};

struct Yield: VariantImpl<Stmt, Yield> {
	Yield( unique_ptr<Expr>&& r ):
		rhs( move( r ) ) {}

	unique_ptr<Expr> rhs;
};

struct Return: VariantImpl<Stmt, Return> {
	Return( unique_ptr<Expr>&& r ):
		retv( move( r ) ) {}

	unique_ptr<Expr> retv;
};

struct Break: VariantImpl<Stmt, Break> {
	Break( unique_ptr<Expr>&& r ):
		retv( move( r ) ) {}

	unique_ptr<Expr> retv;
};

struct While: VariantImpl<Stmt, While> {
	While( unique_ptr<Stmt>&& c, unique_ptr<Stmt>&& b, unique_ptr<Stmt>&& e ):
		cond( move( c ) ), body( move( b ) ), elze( move( e ) ) {}

	unique_ptr<Stmt> cond;
	unique_ptr<Stmt> body;
	unique_ptr<Stmt> elze;
};

struct Bg: VariantImpl<Stmt, Bg> {
	Bg( unique_ptr<Stmt>&& b ):
		body( move( b ) ) {}

	shared_ptr<Stmt> body;
};

struct Sequence: VariantImpl<Stmt, Sequence> {
	Sequence( unique_ptr<Stmt>&& l, unique_ptr<Stmt>&& r ):
		lhs( move( l ) ), rhs( move( r ) ) {}

	unique_ptr<Stmt> lhs;
	unique_ptr<Stmt> rhs;
};

struct Parallel: VariantImpl<Stmt, Parallel> {
	Parallel( unique_ptr<Stmt>&& l, unique_ptr<Stmt>&& r ):
		lhs( move( l ) ), rhs( move( r ) ) {}

	unique_ptr<Stmt> lhs;
	unique_ptr<Stmt> rhs;
};

struct RedirFr: VariantImpl<Stmt, RedirFr> {
	RedirFr( unique_ptr<Stmt>&& b, unique_ptr<Expr>&& f ):
		body( move( b ) ), file( move( f ) ) {}

	unique_ptr<Stmt> body;
	unique_ptr<Expr> file;
};

struct RedirTo: VariantImpl<Stmt, RedirTo> {
	RedirTo( unique_ptr<Stmt>&& b, unique_ptr<Expr>&& f ):
		body( move( b ) ), file( move( f ) ) {}

	unique_ptr<Stmt> body;
	unique_ptr<Expr> file;
};

struct Pipe: VariantImpl<Stmt, Pipe> {
	Pipe( unique_ptr<Stmt>&& l, unique_ptr<Stmt>&& r ):
		lhs( move( l ) ), rhs( move( r ) ) {}

	unique_ptr<Stmt> lhs;
	unique_ptr<Stmt> rhs;
};

struct Zip: VariantImpl<Stmt, Zip> {
	Zip( deque<unique_ptr<Expr>>&& e ):
		exprs( move( e ) ) {}

	deque<unique_ptr<Expr>> exprs;
};

struct Defer: VariantImpl<Stmt, Defer> {
	Defer( unique_ptr<Expr>&& a ):
		args( move( a ) ) {}

	unique_ptr<Expr> args;
};

struct ChDir: VariantImpl<Stmt, ChDir> {
	ChDir( unique_ptr<Expr>&& a ):
		args( move( a ) ) {}

	unique_ptr<Expr> args;
};

struct None: VariantImpl<Stmt, None> {
	None( int r ):
		retv( r ) {}

	int retv;
};


template<class Visitor, class... Args>
void walk( Visitor& visit, Expr* expr, Args&... args ) {
	VSWITCH( expr ) {
		VCASE( Word, e ) {
		}
		VCASE( Home, e ) {
		}
		VCASE( Pair, e ) {
			visit( e->lhs.get(), args... );
			visit( e->rhs.get(), args... );
		}
		VCASE( Concat, e ) {
			visit( e->lhs.get(), args... );
			visit( e->rhs.get(), args... );
		}
		VCASE( Var, e ) {
		}
		VCASE( Subst, e ) {
			visit( e->body.get(), args... );
		}
		VCASE( BinOp, e ) {
			visit( e->lhs.get(), args... );
			visit( e->rhs.get(), args... );
		}
		VCASE( UniOp, e ) {
			visit( e->lhs.get(), args... );
		}
		VCASE( Size, e ) {
			visit( e->var.get(), args... );
		}
		VCASE( Index, e ) {
			visit( e->idx.get(), args... );
			visit( e->var.get(), args... );
		}
		VCASE( Slice, e ) {
			visit( e->bgn.get(), args... );
			visit( e->end.get(), args... );
			visit( e->var.get(), args... );
		}
		VCASE( Null, _ ) {
		}
		VDEFAULT {
			assert( false );
		}
	}
}

template<class Visitor, class... Args>
void walk( Visitor& visit, LeftExpr* expr, Args&... args ) {
	VSWITCH( expr ) {
		VCASE( LeftFix, e ) {
			for( auto& v: e->var ) {
				visit( v.get(), args... );
			}
		}
		VCASE( LeftVar, e ) {
			for( auto& v: e->varL ) {
				visit( v.get(), args... );
			}
			visit( e->varM.get(), args... );
			for( auto& v: e->varR ) {
				visit( v.get(), args... );
			}
		}
		VDEFAULT {
			assert( false );
		}
	}
}

template<class Visitor, class... Args>
void walk( Visitor& visit, Stmt* stmt, Args&... args ) {
	VSWITCH( stmt ) {
		VCASE( Sequence, s ) {
			visit( s->lhs.get(), args... );
			visit( s->rhs.get(), args... );
		}
		VCASE( Parallel, s ) {
			visit( s->lhs.get(), args... );
			visit( s->rhs.get(), args... );
		}
		VCASE( Bg, s ) {
			visit( s->body.get(), args... );
		}
		VCASE( RedirFr, s ) {
			visit( s->file.get(), args... );
			visit( s->body.get(), args... );
		}
		VCASE( RedirTo, s ) {
			visit( s->file.get(), args... );
			visit( s->body.get(), args... );
		}
		VCASE( Command, s ) {
			visit( s->args.get(), args... );
		}
		VCASE( Return, s ) {
			visit( s->retv.get(), args... );
		}
		VCASE( Fun, s ) {
			visit( s->name.get(), args... );
			visit( s->args.get(), args... );
			visit( s->body.get(), args... );
		}
		VCASE( FunDel, s ) {
			visit( s->name.get(), args... );
		}
		VCASE( If, s ) {
			visit( s->cond.get(), args... );
			visit( s->then.get(), args... );
			visit( s->elze.get(), args... );
		}
		VCASE( While, s ) {
			visit( s->cond.get(), args... );
			visit( s->body.get(), args... );
			visit( s->elze.get(), args... );
		}
		VCASE( Break, s ) {
			visit( s->retv.get(), args... );
		}
		VCASE( Let, s ) {
			visit( s->rhs.get(), args... );
			visit( s->lhs.get(), args... );
		}
		VCASE( Fetch, s ) {
			visit( s->lhs.get(), args... );
		}
		VCASE( Yield, s ) {
			visit( s->rhs.get(), args... );
		}
		VCASE( Pipe, s ) {
			visit( s->lhs.get(), args... );
			visit( s->rhs.get(), args... );
		}
		VCASE( Zip, s ) {
			for( auto& e: s->exprs ) {
				visit( e.get(), args... );
			}
		}
		VCASE( Defer, s ) {
			visit( s->args.get(), args... );
		}
		VCASE( ChDir, s ) {
			visit( s->args.get(), args... );
		}
		VCASE( None, s ) {
		}
		VDEFAULT {
			assert( false );
		}
	}
}


} // namespace ast
