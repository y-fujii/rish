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
#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "ast.hpp"
#include "command.hpp"
#include "glob.hpp"
#include "misc.hpp"


struct Global {
	std::map<std::string, std::deque<std::string> > vars;
	std::map<MetaString, ast::Fun*> funs;
};

struct Local {
	Local( Local* o = NULL ): outer( o ) {}

	Local* outer;
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

struct StopException {
};

int evalStmt( ast::Stmt*, Global*, int, int, AtomicBool& );

void evalStmtClose( ast::Stmt* stmt, Global* global, int ifd, int ofd, AtomicBool& stop, bool ic = false, bool oc = false ) {
	evalStmt( stmt, global, ifd, ofd, stop );
	if( ic ) {
		close( ifd );
	}
	if( oc ) {
		close( ofd );
	}
}

template<class DstIter>
DstIter evalExpr( ast::Expr* eb, Global* global, DstIter dst, AtomicBool& stop ) {
	using namespace std;
	using namespace ast;

	if( stop ) {
		throw StopException();
	}

	switch( eb->tag ) {
		MATCH( Expr::tWord ) {
			Word* e = static_cast<Word*>( eb );
			*dst++ = e->word;
		}
		MATCH( Expr::tList ) {
			List* e = static_cast<List*>( eb );
			evalExpr( e->lhs, global, dst, stop );
			evalExpr( e->rhs, global, dst, stop );
		}
		MATCH( Expr::tNull ) {
		}
		MATCH( Expr::tConcat ) {
			Concat* e = static_cast<Concat*>( eb );
			deque<MetaString> lhs;
			deque<MetaString> rhs;
			evalExpr( e->lhs, global, back_inserter( lhs ), stop );
			evalExpr( e->rhs, global, back_inserter( rhs ), stop );
			for( deque<MetaString>::const_iterator i = lhs.begin(); i != lhs.end(); ++i ) {
				for( deque<MetaString>::const_iterator j = rhs.begin(); j != rhs.end(); ++j ) {
					*dst++ = *i + *j;
				}
			}
		}
		MATCH( Expr::tVar ) {
			Var* e = static_cast<Var*>( eb );
			map<string, deque<string> >::const_iterator it = global->vars.find( e->name );
			if( it == global->vars.end() ) {
				throw RuntimeError();
			}
			dst = copy( it->second.begin(), it->second.end(), dst );
		}
		MATCH( Expr::tSubst ) {
			Subst* e = static_cast<Subst*>( eb );

			int fds[2];
			if( pipe( fds ) < 0 ) {
				throw IOError();
			}
			Thread thread( bind( evalStmtClose, e->body, global, 0, fds[1], stop, false, true ) );
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
DstIter evalArgs( ast::Expr* expr, Global* global, DstIter dstIt, AtomicBool& stop ) {
	using namespace std;

	deque<MetaString> tmp;
	evalExpr( expr, global, back_inserter( tmp ), stop );
	return accumulate( tmp.begin(), tmp.end(), dstIt, bind( expandGlob<DstIter>, _2, _1 ) );
}

int evalStmt( ast::Stmt* sb, Global* global, int ifd, int ofd, AtomicBool& stop ) {
	using namespace std;
	using namespace ast;

	if( stop ) {
		throw StopException();
	}

	switch( sb->tag ) {
		MATCH( Stmt::tSequence ) {
			Sequence* s = static_cast<Sequence*>( sb );
			evalStmt( s->lhs, global, ifd, ofd, stop );
			return evalStmt( s->rhs, global, ifd, ofd, stop );
		}
		MATCH( Stmt::tRedirFr ) {
			RedirFr* s = static_cast<RedirFr*>( sb );
			deque<string> args;
			evalArgs( s->file, global, back_inserter( args ), stop );
			int fd = open( args.back().c_str(), O_RDONLY );
			if( fd < 0 ) {
				throw IOError();
			}
			int retv;
			try {
				retv = evalStmt( s->body, global, fd, ofd, stop );
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
			evalArgs( s->file, global, back_inserter( args ), stop );
			int fd = open( args.back().c_str(), O_WRONLY | O_CREAT, 0644 );
			if( fd < 0 ) {
				throw IOError();
			}
			int retv;
			try {
				retv = evalStmt( s->body, global, ifd, fd, stop );
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
			evalArgs( s->args, global, back_inserter( args ), stop );
			
			assert( args.size() > 0 );
			map<MetaString, Fun*>::const_iterator it = global->funs.find( args[0] );
			if( it != global->funs.end() ) {
				try {
					return evalStmt( it->second->body, global, ifd, ofd, stop );
				}
				catch( ReturnException const& e ) {
					return e.retv;
				}
			}
			else {
				return runCommand( args, ifd, ofd );
			}
		}
		MATCH( Stmt::tReturn ) {
			Return* s = static_cast<Return*>( sb );
			deque<string> args;
			evalArgs( s->retv, global, back_inserter( args ), stop );
			int retv;
			if( (istringstream( args.back() ) >> retv).fail() ) {
				return 1;
			}
			throw ReturnException( retv ) ;
		}
		MATCH( Stmt::tFun ) {
			Fun* s = static_cast<Fun*>( sb );
			global->funs[s->name] = s;
			return 0;
		}
		MATCH( Stmt::tIf ) {
			If* s = static_cast<If*>( sb );
			if( evalStmt( s->cond, global, ifd, ofd, stop ) == 0 ) {
				return evalStmt( s->then, global, ifd, ofd, stop );
			}
			else {
				return evalStmt( s->elze, global, ifd, ofd, stop );
			}
		}
		MATCH( Stmt::tWhile ) {
			While* s = static_cast<While*>( sb );
			try {
				while( evalStmt( s->cond, global, ifd, ofd, stop ) == 0 ) {
					evalStmt( s->body, global, ifd, ofd, stop );
				}
			}
			catch( BreakException const& e ) {
				return e.retv;
			}
			return evalStmt( s->elze, global, ifd, ofd, stop );
		}
		MATCH( Stmt::tBreak ) {
			Break* s = static_cast<Break*>( sb );
			deque<string> args;
			evalArgs( s->retv, global, back_inserter( args ), stop );
			int retv;
			if( (istringstream( args.back() ) >> retv).fail() ) {
				return 1;
			}
			throw BreakException( retv ) ;
		}
		MATCH( Stmt::tLetFix ) {
			LetFix* s = static_cast<LetFix*>( sb );
			deque<string> args;
			evalArgs( s->rhs, global, back_inserter( args ), stop );

			Expr* lit = s->lhs;
			for( deque<string>::const_iterator rit = args.begin(); rit != args.end(); ++rit ) {
				if( lit == NULL ) {
					return 1;
				}
				assert( lit->tag == Expr::tList );

				List* l = static_cast<List*>( lit );
				if( l->lhs->tag == Expr::tVar ) {
					Var* e = static_cast<Var*>( l->lhs );
					deque<string>& var = global->vars[e->name];
					var.clear();
					var.push_back( *rit );
				}

				lit = l->rhs;
			}

			return lit != NULL ? 1 : 0;
		}
		MATCH( Stmt::tPipe ) {
			Pipe* s = static_cast<Pipe*>( sb );

			int fds[2];
			if( pipe( fds ) < 0 ) {
				throw IOError();
			}
			Thread thread( bind( evalStmtClose, s->lhs, global, ifd, fds[1], stop, false, true ) );
			evalStmtClose( s->rhs, global, fds[0], ofd, stop, true, false );
			thread.join();

			return 0;
		}
		MATCH( Stmt::tFor ) {
			For* s = static_cast<For*>( sb );

			UnixIStream ifs( ifd, 1 ); // XXX
			while( true ) {
				Expr* it = s->vars;
				while( it->tag == Expr::tList ) {
					List* list = static_cast<List*>( it );
					if( list->lhs->tag == Expr::tVar ) {
						Var* var = static_cast<Var*>( list->lhs );
						string lhs;
						if( !getline( ifs, lhs ) ) {
							return 0;
						}
						deque<string>& rhs = global->vars[var->name];
						rhs.clear();
						rhs.push_back( lhs );
					}
					it = list->rhs;
				}
				evalStmt( s->body, global, ifd, ofd, stop );
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
