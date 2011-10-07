// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <algorithm>
#include <functional>
#include <tr1/functional>
#include <numeric>
#include <deque>
#include <map>
#include <string>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include "ast.hpp"
#include "glob.hpp"
#include "misc.hpp"
#include "unix.hpp"

using namespace std;
using namespace placeholders;


typedef function<int (deque<string> const&, int, int)> Builtin;

struct Global {
	map<string, ast::Fun*> funs;
	map<string, Builtin> builtins;
	//map<string, pair<ast::Fun*, Local*> > funs;
	//pthread_mutex_t lock;
};

struct Local {
	Local( Local* o = nullptr ): outer( o ) {}

	Local* outer;
	map<string, deque<string> > vars;
	deque<deque<string> > defs;
	//pthread_mutex_t lock;
};

struct BreakException {
	BreakException( int r ): retv( r ) {}
	int const retv;
};

struct ReturnException {
	ReturnException( int r ): retv( r ) {}
	int const retv;
};

int evalStmt( ast::Stmt*, Global*, Local*, int, int, bool = false, bool = false );


inline deque<string>& findVariable( string const& name, Local* local ) {
	Local* it = local;
	while( it != nullptr ) {
		map<string, deque<string> >::iterator v = it->vars.find( name );
		if( v != it->vars.end() ) {
			return v->second;
		}
		it = local->outer;
	}

	assert( local != nullptr );
	return local->vars[name];
}

// XXX
template<class Container>
bool assign( ast::VarFix* lhs, Container& rhs, Local* local ) {
	using namespace ast;

	for( size_t i = 0; i < rhs.size(); ++i ) {
		VARIANT_SWITCH( Expr, lhs->var[i] ) {
			VARIANT_CASE( Word, word ) {
				if( word->word != MetaString( rhs[i] ) ) {
					return false;
				}
			}
			VARIANT_DEFAULT {
			}
		}
	}

	//ScopedLock( global->lock );
	//ScopedLock( local->lock );
	for( size_t i = 0; i < rhs.size(); ++i ) {
		VARIANT_SWITCH( Expr, lhs->var[i] ) {
			VARIANT_CASE( Var, tvar ) {
				deque<string>& vvar = findVariable( tvar->name, local );
				vvar.clear();
				vvar.push_back( rhs[i] );
			}
			VARIANT_DEFAULT {
			}
		}
	}

	return true;
}

// XXX
template<class Container>
bool assign( ast::VarVar* lhs, Container& rhs, Local* local ) {
	using namespace ast;

	size_t const lBgn = 0;
	size_t const mBgn = lhs->varL.size();
	size_t const rBgn = rhs.size() - lhs->varR.size();

	for( size_t i = 0; i < lhs->varL.size(); ++i ) {
		VARIANT_SWITCH( Expr, lhs->varL[i] ) {
			VARIANT_CASE( Word, word ) {
				if( word->word != MetaString( rhs[i + lBgn] ) ) {
					return false;
				}
			}
			VARIANT_DEFAULT {
			}
		}
	}
	for( size_t i = 0; i < lhs->varR.size(); ++i ) {
		VARIANT_SWITCH( Expr, lhs->varR[i] ) {
			VARIANT_CASE( Word, word ) {
				if( word->word != MetaString( rhs[i + rBgn] ) ) {
					return false;
				}
			}
			VARIANT_DEFAULT {
			}
		}
	}

	//ScopedLock( global->lock );
	//ScopedLock( local->lock );
	for( size_t i = 0; i < lhs->varL.size(); ++i ) {
		VARIANT_SWITCH( Expr, lhs->varL[i] ) {
			VARIANT_CASE( Var, tvar ) {
				deque<string>& vvar = findVariable( tvar->name, local );
				vvar.clear();
				vvar.push_back( rhs[i + lBgn] );
			}
			VARIANT_DEFAULT {
			}
		}
	}
	deque<string>& vvar = findVariable( lhs->varM->name, local );
	vvar.clear();
	copy( &rhs[mBgn], &rhs[rBgn], back_inserter( vvar ) );
	for( size_t i = 0; i < lhs->varR.size(); ++i ) {
		VARIANT_SWITCH( Expr, lhs->varR[i] ) {
			VARIANT_CASE( Var, tvar ) {
				deque<string>& vvar = findVariable( tvar->name, local );
				vvar.clear();
				vvar.push_back( rhs[i + rBgn] );
			}
			VARIANT_DEFAULT {
			}
		}
	}

	return true;
}

template<class Container>
bool assign( ast::LeftExpr* lhsb, Container& rhs, Local* local ) {
	using namespace ast;

	VARIANT_SWITCH( LeftExpr, lhsb ) {
		VARIANT_CASE( VarFix, lhs ) {
			if( rhs.size() != lhs->var.size() ) {
				return false;
			}
			return assign( lhs, rhs, local );
		}
		VARIANT_CASE( VarVar, lhs ) {
			if( rhs.size() < lhs->varL.size() + lhs->varR.size() ) {
				return false;
			}
			return assign( lhs, rhs, local );
		}
		VARIANT_DEFAULT {
			assert( false );
		}
	}

	assert( false );
	return false;
}

template<class DstIter>
DstIter evalExpr( ast::Expr* eb, Global* global, Local* local, int ifd, DstIter dst ) {
	using namespace ast;

	Thread::checkIntr();

	VARIANT_SWITCH( Expr, eb ) {
		VARIANT_CASE( Word, e ) {
			*dst++ = e->word;
		}
		VARIANT_CASE( Pair, e ) {
			evalExpr( e->lhs, global, local, ifd, dst );
			evalExpr( e->rhs, global, local, ifd, dst );
		}
		VARIANT_CASE( Null, _ ) {
		}
		VARIANT_CASE( Concat, e ) {
			deque<MetaString> lhs;
			deque<MetaString> rhs;
			evalExpr( e->lhs, global, local, ifd, back_inserter( lhs ) );
			evalExpr( e->rhs, global, local, ifd, back_inserter( rhs ) );
			for( deque<MetaString>::const_iterator i = lhs.begin(); i != lhs.end(); ++i ) {
				for( deque<MetaString>::const_iterator j = rhs.begin(); j != rhs.end(); ++j ) {
					*dst++ = *i + *j;
				}
			}
		}
		VARIANT_CASE( Var, e ) {
			deque<string>& val = findVariable( e->name, local );
			dst = copy( val.begin(), val.end(), dst );
		}
		VARIANT_CASE( Subst, e ) {
			int fds[2];
			checkSysCall( pipe( fds ) );
			checkSysCall( fcntl( fds[0], F_SETFD, FD_CLOEXEC ) );
			checkSysCall( fcntl( fds[1], F_SETFD, FD_CLOEXEC ) );
			Thread thread( bind( evalStmt, e->body, global, local, ifd, fds[1], false, true ) );
			try {
				{
					ScopeExit closer( bind( close, fds[0] ) );
					UnixIStream ifs( fds[0] );
					string buf;
					while( getline( ifs, buf ) ) {
						*dst++ = buf;
					}
				}
				thread.join();
			}
			catch( ... ) {
				try {
					thread.interrupt();
					thread.join();
				}
				catch ( ... ) {}
				throw;
			}
		}
		VARIANT_DEFAULT {
			assert( false );
		}
	}
	return dst;
}

int execCommand( deque<string>& args, Global* global, int ifd, int ofd ) {
	map<string, ast::Fun*>::const_iterator fit = global->funs.find( args[0] );
	if( fit != global->funs.end() ) {
		Local local;
		int retv;
		try {
			args.pop_front();
			if( assign( fit->second->args, args, &local ) ) {
				retv = evalStmt( fit->second->body, global, &local, ifd, ofd );
			}
			else {
				return 1; // to be implemented
			}
		}
		catch( ReturnException const& e ) {
			retv = e.retv;
		}

		for( deque<deque<string> >::reverse_iterator it = local.defs.rbegin(); it != local.defs.rend(); ++it ) {
			execCommand( *it, global, ifd, ofd );
		}

		return retv;
	}

	map<string, Builtin>::const_iterator bit = global->builtins.find( args[0] );
	if( bit != global->builtins.end() ) {
		args.pop_front();
		return bit->second( args, ifd, ofd );
	}

	return forkExec( args, ifd, ofd );
}

template<class DstIter>
DstIter evalArgs( ast::Expr* expr, Global* global, Local* local, int ifd, DstIter dstIt ) {
	deque<MetaString> tmp;
	evalExpr( expr, global, local, ifd, back_inserter( tmp ) );
	return accumulate( tmp.begin(), tmp.end(), dstIt, bind( expandGlob<DstIter>, _2, _1 ) );
}

int evalStmt( ast::Stmt* sb, Global* global, Local* local, int ifd, int ofd, bool ic, bool oc ) {
	using namespace ast;

	ScopeExit icloser( bind( close, ifd ), ic );
	ScopeExit ocloser( bind( close, ofd ), oc );

	Thread::checkIntr();

	//try {
	VARIANT_SWITCH( Stmt, sb ) {
		VARIANT_CASE( Sequence, s ) {
			evalStmt( s->lhs, global, local, ifd, ofd );
			return evalStmt( s->rhs, global, local, ifd, ofd );
		}
		VARIANT_CASE( Bg, s ) {
			// XXX: detach?
			Thread thread( bind( evalStmt, s->body, global, local, ifd, ofd, false, false ) );
			return 0;
		}
		VARIANT_CASE( RedirFr, s ) {
			deque<string> args;
			evalArgs( s->file, global, local, ifd, back_inserter( args ) );
			int fd = open( args.back().c_str(), O_RDONLY );
			checkSysCall( fd );
			checkSysCall( fcntl( fd, F_SETFD, FD_CLOEXEC ) );
			return evalStmt( s->body, global, local, fd, ofd, true, false );
		}
		VARIANT_CASE( RedirTo, s ) {
			deque<string> args;
			evalArgs( s->file, global, local, ifd, back_inserter( args ) );
			int fd = open( args.back().c_str(), O_WRONLY | O_CREAT, 0644 );
			checkSysCall( fd );
			checkSysCall( fcntl( fd, F_SETFD, FD_CLOEXEC ) );
			return evalStmt( s->body, global, local, ifd, fd, false, true );
		}
		VARIANT_CASE( Command, s ) {
			deque<string> args;
			evalArgs( s->args, global, local, ifd, back_inserter( args ) );
			if( args.size() == 0 ) {
				return 0;
			}

			return execCommand( args, global, ifd, ofd );
		}
		VARIANT_CASE( Return, s ) {
			deque<string> args;
			evalArgs( s->retv, global, local, ifd, back_inserter( args ) );
			if( args.size() != 1 ) {
				return 1;
			}
			int retv;
			if( (istringstream( args[0] ) >> retv).fail() ) {
				return 1;
			}
			throw ReturnException( retv ) ;
		}
		VARIANT_CASE( Fun, s ) {
			deque<string> args;
			evalArgs( s->name, global, local, ifd, back_inserter( args ) );
			if( args.size() != 1 ) {
				return 1;
			}
			global->funs[args[0]] = s;
			return 0;
		}
		VARIANT_CASE( If, s ) {
			if( evalStmt( s->cond, global, local, ifd, ofd ) == 0 ) {
				return evalStmt( s->then, global, local, ifd, ofd );
			}
			else {
				return evalStmt( s->elze, global, local, ifd, ofd );
			}
		}
		VARIANT_CASE( While, s ) {
			try {
				while( evalStmt( s->cond, global, local, ifd, ofd ) == 0 ) {
					evalStmt( s->body, global, local, ifd, ofd );
				}
			}
			catch( BreakException const& e ) {
				return e.retv;
			}
			return evalStmt( s->elze, global, local, ifd, ofd );
		}
		VARIANT_CASE( Break, s ) {
			deque<string> args;
			evalArgs( s->retv, global, local, ifd, back_inserter( args ) );
			if( args.size() != 1 ) {
				return 1;
			}
			int retv;
			if( (istringstream( args[0] ) >> retv).fail() ) {
				return 1;
			}
			throw BreakException( retv );
		}
		VARIANT_CASE( Let, s ) {
			deque<string> vals;
			evalArgs( s->rhs, global, local, ifd, back_inserter( vals ) );
			return assign( s->lhs, vals, local ) ? 0 : 1;
		}
		VARIANT_CASE( Fetch, s ) {
			VARIANT_SWITCH( LeftExpr, s->lhs ) {
				VARIANT_CASE( VarFix, lhs ) {
					UnixIStream ifs( ifd, 1 );
					deque<string> rhs( lhs->var.size() );
					for( deque<string>::iterator it = rhs.begin(); it != rhs.end(); ++it ) {
						if( !getline( ifs, *it ) ) {
							return 1;
						}
					}

					return assign( lhs, rhs, local ) ? 0 : 1;
				}
				VARIANT_CASE( VarVar, lhs ) {
					deque<string> rhs;
					UnixIStream ifs( ifd );
					string buf;
					while( getline( ifs, buf ) ) {
						rhs.push_back( buf );
					}
					if( rhs.size() < lhs->varL.size() + lhs->varR.size() ) {
						return 1;
					}

					return assign( lhs, rhs, local ) ? 0 : 1;
				}
				VARIANT_DEFAULT {
					assert( false );
				}
			}
		}
		VARIANT_CASE( Yield, s ) {
			deque<string> vals;
			evalArgs( s->rhs, global, local, ifd, back_inserter( vals ) );
			ostringstream buf;
			for( deque<string>::const_iterator it = vals.begin(); it != vals.end(); ++it ) {
				buf << *it << '\n';
			}
			writeAll( ofd, buf.str() );
			return 0;
		}
		VARIANT_CASE( Pipe, s ) {
			int fds[2];
			checkSysCall( pipe( fds ) );
			checkSysCall( fcntl( fds[0], F_SETFD, FD_CLOEXEC ) );
			checkSysCall( fcntl( fds[1], F_SETFD, FD_CLOEXEC ) );
			Thread thread( bind( evalStmt, s->lhs, global, local, ifd, fds[1], false, true ) );
			try {
				int retv = evalStmt( s->rhs, global, local, fds[0], ofd, true, false );
				thread.join();
				return retv;
			}
			catch( ... ) {
				try {
					thread.interrupt();
					thread.join();
				}
				catch ( ... ) {}
				throw;
			}
		}
		VARIANT_CASE( Defer, s ) {
			local->defs.push_back( deque<string>() );
			evalArgs( s->args, global, local, ifd, back_inserter( local->defs.back() ) );
			return 0;
		}
		VARIANT_CASE( None, s ) {
			return s->retv;
		}
		VARIANT_DEFAULT {
			assert( false );
		}
	}
	/*
	}
	catch( IOError const& ) {
		return 1;
	}
	*/

	assert( false );
	return -1;
}
