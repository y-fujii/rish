#pragma once

#include "config.hpp"
#include <iterator>
#include <deque>
#include <map>
#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <stdexcept>
#include <numeric>
#include <tr1/functional>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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

int evalStatement( ast::Statement*, Global*, int, int );

// XXX
template<class DstIter>
DstIter readList( int ifd, DstIter dst ) {
	char c;
	string buf;
	while( read( ifd, &c, 1 ) != 0 ) {
		if( c == '\n' ) {
			*dst++ = buf;
			buf.clear();
		}
		else {
			buf += c;
		}
	}
	if( buf.size() > 0 ) {
		*dst++ = buf;
	}
	return dst;
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
		MATCH( Expr::tNull ) {
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
			map<string, deque<string> >::const_iterator it = global->vars.find( *e->name );
			if( it == global->vars.end() ) {
				throw runtime_error( "Variable " + *e->name + " is not defined." );
			}
			dst = copy( it->second.begin(), it->second.end(), dst );
		}
		MATCH( Expr::tSubst ) {
			Subst* e = static_cast<Subst*>( eb );

			int fds[2];
			if( pipe( fds ) < 0 ) {
				throw IOError();
			}
			pid_t pid = fork();
			if( pid < 0 ) {
				throw IOError();
			}
			if( pid == 0 ) {
				close( 0 );
				close( fds[0] );
				evalStatement( e->body, global, 0, fds[1] );
				exit( 0 );
			}
			else {
				close( fds[1] );
				readList( fds[0], dst );
			}
			waitpid( pid, NULL, 0 );
		}
		OTHERWISE {
			assert( false );
		}
	}
	return dst;
}

template<class DstIter>
DstIter evalArgs( ast::Expr* expr, Global* global, DstIter dstIt ) {
	using namespace std;
	using namespace std::tr1;
	using namespace std::tr1::placeholders;

	deque<MetaString> tmp;
	evalExpr( expr, global, back_inserter( tmp ) );
	return accumulate( tmp.begin(), tmp.end(), dstIt, bind( expandGlob<DstIter>, _2, _1 ) );
}

struct EvalStatmentRunner {
	EvalStatmentRunner( ast::Statement* s, Global* g, int i, int o ):
		_statement( s ), _global( g ), _ifd( i ), _ofd( o ) {
		if( pthread_create( &_thread, NULL, callback, this ) != 0 ) {
			throw IOError();
		}
	}

	int join() {
		void* retv;
		if( pthread_join( _thread, &retv ) == 0 ) {
			throw IOError();
		}
		return int( reinterpret_cast<uintptr_t>( retv ) );
	}

	private:
		static void* callback( void* arg ) {
			EvalStatmentRunner* self = reinterpret_cast<EvalStatmentRunner*>( arg );
			return reinterpret_cast<void*>(
				evalStatement( self->_statement, self->_global, self->_ifd, self->_ofd )
			);
		}

		pthread_t _thread;
		ast::Statement* _statement;
		Global* _global;
		int _ifd;
		int _ofd;
};

int evalStatement( ast::Statement* sb, Global* global, int ifd, int ofd ) {
	using namespace std;
	using namespace ast;

	switch( sb->tag ) {
		MATCH( Statement::tSequence ) {
			Sequence* s = static_cast<Sequence*>( sb );
			evalStatement( s->lhs, global, ifd, ofd );
			return evalStatement( s->rhs, global, ifd, ofd );
		}
		MATCH( Statement::tRedirFr ) {
			RedirFr* s = static_cast<RedirFr*>( sb );
			deque<string> args;
			evalArgs( s->file, global, back_inserter( args ) );
			int fd = open( args.back().c_str(), O_RDONLY );
			int retv;
			try {
				retv = evalStatement( s->body, global, fd, ofd );
			}
			catch( ... ) {
				close( fd );
				throw;
			}
			close( fd );
			return retv;
		}
		MATCH( Statement::tRedirTo ) {
			RedirFr* s = static_cast<RedirFr*>( sb );
			deque<string> args;
			evalArgs( s->file, global, back_inserter( args ) );
			int fd = open( args.back().c_str(), O_WRONLY | O_CREAT, 0644 );
			int retv;
			try {
				retv = evalStatement( s->body, global, ifd, fd );
			}
			catch( ... ) {
				close( fd );
				throw;
			}
			close( fd );
			return retv;
		}
		MATCH( Statement::tCommand ) {
			Command* s = static_cast<Command*>( sb );
			deque<string> args;
			evalArgs( s->args, global, back_inserter( args ) );
			
			/*
			assert( args.size() > 0 );
			Env::iterator it = env->funcs.find( c->name );
			if( it != env->funcs.end() ) {
				scope_ptr<Local> local = new Local();
				unify( *local, it->args, args.begin(), args.end() );
				evalStatement( it->body, *local, ifd, ofd );
			}
			else {
				runCommand( args );
			}
			*/
			return runCommand( args, ifd, ofd );
		}
		/*
		MATCH( Statement::tFun ) {
			Fun* s = static_cast<Fun*>( sb );
			env->funcs[*f->name] = s;
		}
		*/
		MATCH( Statement::tIf ) {
			If* s = static_cast<If*>( sb );
			if( evalStatement( s->cond, global, ifd, ofd ) == 0 ) {
				return evalStatement( s->then, global, ifd, ofd );
			}
			else {
				return evalStatement( s->elze, global, ifd, ofd );
			}
		}
		MATCH( Statement::tWhile ) {
			While* s = static_cast<While*>( sb );
			try {
				while( evalStatement( s->cond, global, ifd, ofd ) == 0 ) {
					evalStatement( s->body, global, ifd, ofd );
				}
			}
			catch( BreakException const& br ) {
				return br.retv;
			}
			return evalStatement( s->elze, global, ifd, ofd );
		}
		MATCH( Statement::tBreak ) {
			Break* s = static_cast<Break*>( sb );
			deque<string> args;
			evalArgs( s->retv, global, back_inserter( args ) );
			int retv;
			if( (istringstream( args.back() ) >> retv).fail() ) {
				return 1;
			}
			throw BreakException( retv ) ;
		}
		MATCH( Statement::tLetFix ) {
			LetFix* s = static_cast<LetFix*>( sb );
			deque<string> args;
			evalArgs( s->rhs, global, back_inserter( args ) );

			Expr* lit = s->lhs;
			for( deque<string>::const_iterator rit = args.begin(); rit != args.end(); ++rit ) {
				if( lit->tag == Expr::tNull ) {
					return 1;
				}
				assert( lit->tag == Expr::tList );

				List* l = static_cast<List*>( lit );
				if( l->lhs->tag == Expr::tVar ) {
					Var* e = static_cast<Var*>( l->lhs );
					global->vars[*e->name].clear();
					global->vars[*e->name].push_back( *rit );
				}

				lit = l->rhs;
			}

			return lit->tag != Expr::tNull ? 1 : 0;
		}
		MATCH( Statement::tPipe ) {
			Pipe* s = static_cast<Pipe*>( sb );
			int fds[2];
			if( pipe( fds ) < 0 ) {
				throw IOError();
			}

			pid_t pid = fork();
			if( pid < 0 ) {
				throw IOError();
			}
			if( pid == 0 ) {
				close( fds[0] );
				evalStatement( s->lhs, global, ifd, fds[1] );
				exit( 0 );
			}
			else {
				close( fds[1] );
				evalStatement( s->rhs, global, fds[0], ofd );
			}
			waitpid( pid, NULL, 0 );

			return 0;
		}
		MATCH( Statement::tFor ) {
			For* s = static_cast<For*>( sb );
			// XXX
			char c;
			string buf;
			while( read( ifd, &c, 1 ) != 0 ) {
				if( c == '\n' ) {
					global->vars[*s->var].clear();
					global->vars[*s->var].push_back( buf );
					evalStatement( s->body, global, ifd, ofd );
					buf.clear();
				}
				else {
					buf += c;
				}
			}
			if( buf.size() > 0 ) {
				global->vars[*s->var].clear();
				global->vars[*s->var].push_back( buf );
				evalStatement( s->body, global, ifd, ofd );
			}
			return 0;
		}
		MATCH( Statement::tNone ) {
			return 0;
		}
		OTHERWISE {
			assert( false );
		}
	}
	assert( false );
}
