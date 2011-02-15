#pragma once

#include "config.hpp"
#include <algorithm>
#include <functional>
#include <tr1/functional>
#include <numeric>
#include <deque>
#include <map>
#include <string>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "ast.hpp"
#include "command.hpp"
#include "glob.hpp"
#include "misc.hpp"


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

int evalStmt( ast::Stmt*, Global*, int, int );

void evalStmtClose( ast::Stmt* stmt, Global* global, int ifd, int ofd, bool ic = false, bool oc = false ) {
	evalStmt( stmt, global, ifd, ofd );
	if( ic ) {
		close( ifd );
	}
	if( oc ) {
		close( ofd );
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
			Thread thread( bind( evalStmtClose, e->body, global, 0, fds[1], false, true ) );
			{
				UnixIStream ifs( fds[0] );
				while( !ifs.eof() ) {
					string buf;
					getline( ifs, buf );
					*dst++ = buf;
				}
			}
			close( fds[0] );
			thread.join();
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

	deque<MetaString> tmp;
	evalExpr( expr, global, back_inserter( tmp ) );
	return accumulate( tmp.begin(), tmp.end(), dstIt, bind( expandGlob<DstIter>, _2, _1 ) );
}

int evalStmt( ast::Stmt* sb, Global* global, int ifd, int ofd ) {
	using namespace std;
	using namespace ast;

	switch( sb->tag ) {
		MATCH( Stmt::tSequence ) {
			Sequence* s = static_cast<Sequence*>( sb );
			evalStmt( s->lhs, global, ifd, ofd );
			return evalStmt( s->rhs, global, ifd, ofd );
		}
		MATCH( Stmt::tRedirFr ) {
			RedirFr* s = static_cast<RedirFr*>( sb );
			deque<string> args;
			evalArgs( s->file, global, back_inserter( args ) );
			int fd = open( args.back().c_str(), O_RDONLY );
			int retv;
			try {
				retv = evalStmt( s->body, global, fd, ofd );
			}
			catch( ... ) {
				close( fd );
				throw;
			}
			close( fd );
			return retv;
		}
		MATCH( Stmt::tRedirTo ) {
			RedirFr* s = static_cast<RedirFr*>( sb );
			deque<string> args;
			evalArgs( s->file, global, back_inserter( args ) );
			int fd = open( args.back().c_str(), O_WRONLY | O_CREAT, 0644 );
			int retv;
			try {
				retv = evalStmt( s->body, global, ifd, fd );
			}
			catch( ... ) {
				close( fd );
				throw;
			}
			close( fd );
			return retv;
		}
		MATCH( Stmt::tCommand ) {
			Command* s = static_cast<Command*>( sb );
			deque<string> args;
			evalArgs( s->args, global, back_inserter( args ) );
			
			/*
			assert( args.size() > 0 );
			Env::iterator it = env->funcs.find( c->name );
			if( it != env->funcs.end() ) {
				scope_ptr<Local> local = new Local();
				unify( *local, it->args, args.begin(), args.end() );
				evalStmt( it->body, *local, ifd, ofd );
			}
			else {
				runCommand( args );
			}
			*/
			return runCommand( args, ifd, ofd );
		}
		/*
		MATCH( Stmt::tFun ) {
			Fun* s = static_cast<Fun*>( sb );
			env->funcs[*f->name] = s;
		}
		*/
		MATCH( Stmt::tIf ) {
			If* s = static_cast<If*>( sb );
			if( evalStmt( s->cond, global, ifd, ofd ) == 0 ) {
				return evalStmt( s->then, global, ifd, ofd );
			}
			else {
				return evalStmt( s->elze, global, ifd, ofd );
			}
		}
		MATCH( Stmt::tWhile ) {
			While* s = static_cast<While*>( sb );
			try {
				while( evalStmt( s->cond, global, ifd, ofd ) == 0 ) {
					evalStmt( s->body, global, ifd, ofd );
				}
			}
			catch( BreakException const& br ) {
				return br.retv;
			}
			return evalStmt( s->elze, global, ifd, ofd );
		}
		MATCH( Stmt::tBreak ) {
			Break* s = static_cast<Break*>( sb );
			deque<string> args;
			evalArgs( s->retv, global, back_inserter( args ) );
			int retv;
			if( (istringstream( args.back() ) >> retv).fail() ) {
				return 1;
			}
			throw BreakException( retv ) ;
		}
		MATCH( Stmt::tLetFix ) {
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
					deque<string>& var = global->vars[*e->name];
					var.clear();
					var.push_back( *rit );
				}

				lit = l->rhs;
			}

			return lit->tag != Expr::tNull ? 1 : 0;
		}
		MATCH( Stmt::tPipe ) {
			Pipe* s = static_cast<Pipe*>( sb );

			int fds[2];
			if( pipe( fds ) < 0 ) {
				throw IOError();
			}
			Thread thread( bind( evalStmtClose, s->lhs, global, ifd, fds[1], false, true ) );
			evalStmtClose( s->rhs, global, fds[0], ofd, true, false );
			thread.join();

			return 0;
		}
		MATCH( Stmt::tFor ) {
			For* s = static_cast<For*>( sb );

			deque<string>& var = global->vars[*s->var];
			UnixIStream ifs( ifd );
			while( !ifs.eof() ) {
				string line;
				getline( ifs, line );
				var.clear();
				var.push_back( line );
				evalStmt( s->body, global, ifd, ofd );
			}

			return 0;
		}
		MATCH( Stmt::tNone ) {
			return 0;
		}
		OTHERWISE {
			assert( false );
		}
	}
	assert( false );
}
