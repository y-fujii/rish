#pragma once

#include <iterator>
#include <deque>
#include <map>
#include <string>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>
#include "ast.hpp"
#include "command.hpp"
#include "misc.hpp"
#include "glob.hpp"


struct Global {
	std::map<std::string, std::deque<std::string> > vars;
	//std::map<std::string, Fun*> funs;
};

struct Local {
	std::map<std::string, std::deque<std::string> > vars;
};

struct BreakException {
	BreakException( int r ): retv( r ) {}
	virtual ~BreakException() {}

	int const retv;
};

struct ReturnException {
	ReturnException( int r ): retv( r ) {}
	virtual ~ReturnException() {}

	int const retv;
};

template<class RhsIter>
bool unify( ast::VarLhs* lhsb, RhsIter rhsBgn, RhsIter rhsEnd, Global* global ) {
	using namespace ast;
	assert( distance( rhsBgn, rhsEnd ) >= 0 );

	switch( lhsb->tag ) {
		MATCH( VarLhs::tVarList ) {
			VarList* lhs = static_cast<VarList*>( lhsb );
			if( lhs->vars->size() != size_t( distance( rhsBgn, rhsEnd ) ) ) {
				return false;
			}

			RhsIter rit = rhsBgn;
			deque<Expr*>::iterator lit = lhs->vars->begin();
			while( rit != rhsEnd ) {
				if( (*lit)->tag == Expr::tVar ) {
					Var* e = static_cast<Var*>( *lit );
					global->vars[toString( *e->name )].clear();
					global->vars[toString( *e->name )].push_back( toString( *rit ) );
				}
				++rit;
				++lit;
			}
			return true;
		}
		MATCH( VarLhs::tVarStar ) {
			VarStar* lhs = static_cast<VarStar*>( lhsb );
			if( lhs->head->size() + lhs->tail->size() < size_t( distance( rhsBgn, rhsEnd ) ) ) {
				return false;
			}
			return true;
		}
		OTHERWISE {
			assert( false );
			return false;
		}
	}
}

template<class DstIter>
DstIter evalExpr( ast::Expr* eb, Global* global, DstIter dst ) {
	using namespace std;
	using namespace ast;

	switch( eb->tag ) {
		MATCH( Expr::tWord ) {
			Word* e = static_cast<Word*>( eb );
			*dst++ = *e->word;
		}
		MATCH( Expr::tList ) {
			List* e = static_cast<List*>( eb );
			evalExpr( e->lhs, global, dst );
			evalExpr( e->rhs, global, dst );
		}
		MATCH( Expr::tConcat ) {
			Concat* e = static_cast<Concat*>( eb );
			deque<MetaString> lhs;
			deque<MetaString> rhs;
			evalExpr( e->lhs, global, back_inserter( lhs ) );
			evalExpr( e->rhs, global, back_inserter( rhs ) );
			for( deque<MetaString>::const_iterator i = lhs.begin(); i != lhs.end(); ++i ) {
				for( deque<MetaString>::const_iterator j = rhs.begin(); j != rhs.end(); ++j ) {
					*dst++ = *i + *j;
				}
			}
		}
		MATCH( Expr::tVar ) {
			Var* e = static_cast<Var*>( eb );
			map<string, deque<string> >::const_iterator it = global->vars.find( toString( *e->name ) );
			if( it == global->vars.end() ) {
				throw runtime_error( "The variable " + toString( *e->name )+ " is not defined." );
			}
			dst = copy( it->second.begin(), it->second.end(), dst );
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
	return dst;
}

int evalStatement( ast::Statement* sb, Global* global ) {
	using namespace std;
	using namespace ast;

	switch( sb->tag ) {
		MATCH( Statement::tSequence ) {
			Sequence* s = static_cast<Sequence*>( sb );
			evalStatement( s->lhs, global );
			return evalStatement( s->rhs, global );
		}
		MATCH( Statement::tRedir ) {
			Redir* s = static_cast<Redir*>( sb );
			return evalStatement( s->body, global );
		}
		MATCH( Statement::tCommand ) {
			Command* s = static_cast<Command*>( sb );
			deque<MetaString> vals;
			deque<string> args;
			evalExpr( s->args, global, back_inserter( vals ) );
			for( deque<MetaString>::const_iterator it = vals.begin(); it != vals.end(); ++it ) {
				//expandGlob( "./", *it, back_inserter( args ) );
				args.push_back( toString( *it ) );
			}
			
			/*
			assert( args.size() > 0 );
			Env::iterator it = env->funcs.find( c->name );
			if( it != env->funcs.end() ) {
				scope_ptr<Local> local = new Local();
				unify( *local, it->args, args.begin(), args.end() );
				evalStatement( it->body, *local );
			}
			else {
				runCommand( args );
			}
			*/
			return runCommand( args );
		}
		/*
		MATCH( Statement::tFun ) {
			Fun* s = static_cast<Fun*>( sb );
			env->funcs[*f->name] = s;
		}
		*/
		MATCH( Statement::tIf ) {
			If* s = static_cast<If*>( sb );
			if( evalStatement( s->cond, global ) == 0 ) {
				return evalStatement( s->then, global );
			}
			else {
				return evalStatement( s->elze, global );
			}
		}
		MATCH( Statement::tWhile ) {
			While* s = static_cast<While*>( sb );
			try {
				while( evalStatement( s->cond, global ) == 0 ) {
					evalStatement( s->body, global );
				}
			}
			catch( BreakException const& br ) {
				return br.retv;
			}
			return evalStatement( s->elze, global );
		}
		MATCH( Statement::tBreak ) {
			Break* s = static_cast<Break*>( sb );
			deque<MetaString> tmp;
			evalExpr( s->retv, global, back_inserter( tmp ) );
			int retv;
			if( (istringstream( toString( tmp.back() ) ) >> retv).fail() ) {
				return 1;
			}
			throw BreakException( retv ) ;
		}
		MATCH( Statement::tLet ) {
			Let* s = static_cast<Let*>( sb );
			deque<MetaString> rhs;
			evalExpr( s->rhs, global, back_inserter( rhs ) );
			return unify( s->lhs, rhs.begin(), rhs.end(), global ) ? 0 : 1;
		}
		MATCH( Statement::tNone ) {
			return 0;
		}
		OTHERWISE {
			assert( false );
		}
	}
	assert( false );
	return 1;
}
