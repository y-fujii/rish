#pragma once

#include <iterator>
#include <deque>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include "ast.hpp"
#include "command.hpp"
#include "misc.hpp"


template<class DstIter>
void evalValue( DstIter dst, ast::Expr* e /* , Environment* env */ ) {
	using namespace std;
	using namespace ast;

	switch( e->tag ) {
		MATCH( Expr::tWord ) {
			Word* w = static_cast<Word*>( e );
			*dst++ = *w->word;
		}
		MATCH( Expr::tList ) {
			List* l = static_cast<List*>( e );
			evalValue( dst, l->lhs );
			evalValue( dst, l->rhs );
		}
		MATCH( Expr::tConcat ) {
			Concat* c = static_cast<Concat*>( e );
			deque<string> lhs;
			deque<string> rhs;
			evalValue( back_inserter( lhs ), c->lhs );
			evalValue( back_inserter( rhs ), c->rhs );
			for( deque<string>::const_iterator i = lhs.begin(); i != lhs.end(); ++i ) {
				for( deque<string>::const_iterator j = rhs.begin(); j != rhs.end(); ++j ) {
					*dst++ = *i + *j;
				}
			}
		}
		MATCH( Expr::tVar ) {
			/*
			Var* v = static_cast<Var*>( e );
			Environment::iterator it = env->find( v->name );
			if( it == env->end() ) {
				throw exception();
			}
			*dst++ = *it->second;
			*/
		}
		MATCH( Expr::tSubst ) {
		}
		MATCH( Expr::tNull ) {
		}
		/*
		OTHERWISE {
			assert( false );
		}
		*/
	}
}

void evalStatement( ast::Statement* s ) {
	using namespace std;
	using namespace ast;

	switch( s->tag ) {
		MATCH( Statement::tSequence ) {
			Sequence* t = static_cast<Sequence*>( s );
			evalStatement( t->lhs );
			evalStatement( t->rhs );
		}
		MATCH( Statement::tRedir ) {
			Redir* r = static_cast<Redir*>( s );
			evalStatement( r->body );
		}
		MATCH( Statement::tCommand ) {
			Command* c = static_cast<Command*>( s );
			deque<string> args;
			evalValue( back_inserter( args ), c->args );
			runCommand( args );
		}
		MATCH( Statement::tNone ) {
		}
		OTHERWISE {
			assert( false );
		}
	}
}
