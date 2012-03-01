// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <algorithm>
#include <functional>
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

int evalStmt( ast::Stmt*, Global*, Local*, int, int );


inline deque<string>& findVariable( string const& name, Local* local ) {
	auto it = local;
	while( it != nullptr ) {
		auto v = it->vars.find( name );
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
		VSWITCH( lhs->var[i] ) {
			VCASE( Word, word ) {
				if( word->word != MetaString( rhs[i] ) ) {
					return false;
				}
			}
			VDEFAULT {
			}
		}
	}

	//ScopedLock( global->lock );
	//ScopedLock( local->lock );
	for( size_t i = 0; i < rhs.size(); ++i ) {
		VSWITCH( lhs->var[i] ) {
			VCASE( Var, tvar ) {
				auto& vvar = findVariable( tvar->name, local );
				vvar.clear();
				vvar.push_back( rhs[i] );
			}
			VDEFAULT {
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
		VSWITCH( lhs->varL[i] ) {
			VCASE( Word, word ) {
				if( word->word != MetaString( rhs[i + lBgn] ) ) {
					return false;
				}
			}
			VDEFAULT {
			}
		}
	}
	for( size_t i = 0; i < lhs->varR.size(); ++i ) {
		VSWITCH( lhs->varR[i] ) {
			VCASE( Word, word ) {
				if( word->word != MetaString( rhs[i + rBgn] ) ) {
					return false;
				}
			}
			VDEFAULT {
			}
		}
	}

	//ScopedLock( global->lock );
	//ScopedLock( local->lock );
	for( size_t i = 0; i < lhs->varL.size(); ++i ) {
		VSWITCH( lhs->varL[i] ) {
			VCASE( Var, tvar ) {
				auto& vvar = findVariable( tvar->name, local );
				vvar.clear();
				vvar.push_back( rhs[i + lBgn] );
			}
			VDEFAULT {
			}
		}
	}
	auto& vvar = findVariable( lhs->varM->name, local );
	vvar.clear();
	copy( &rhs[mBgn], &rhs[rBgn], back_inserter( vvar ) );
	for( size_t i = 0; i < lhs->varR.size(); ++i ) {
		VSWITCH( lhs->varR[i] ) {
			VCASE( Var, tvar ) {
				auto& vvar = findVariable( tvar->name, local );
				vvar.clear();
				vvar.push_back( rhs[i + rBgn] );
			}
			VDEFAULT {
			}
		}
	}

	return true;
}

template<class Container>
bool assign( ast::LeftExpr* lhsb, Container& rhs, Local* local ) {
	using namespace ast;

	VSWITCH( lhsb ) {
		VCASE( VarFix, lhs ) {
			if( rhs.size() != lhs->var.size() ) {
				return false;
			}
			return assign( lhs, rhs, local );
		}
		VCASE( VarVar, lhs ) {
			if( rhs.size() < lhs->varL.size() + lhs->varR.size() ) {
				return false;
			}
			return assign( lhs, rhs, local );
		}
		VDEFAULT {
			assert( false );
		}
	}

	assert( false );
	return false;
}

template<class DstIter>
DstIter evalExpr( ast::Expr* expr, Global* global, Local* local, int ifd, DstIter dst ) {
	using namespace ast;

	Thread::checkIntr();

	VSWITCH( expr ) {
		VCASE( Word, e ) {
			*dst++ = e->word;
		}
		VCASE( Pair, e ) {
			evalExpr( e->lhs, global, local, ifd, dst );
			evalExpr( e->rhs, global, local, ifd, dst );
		}
		VCASE( Concat, e ) {
			deque<MetaString> lhs;
			deque<MetaString> rhs;
			evalExpr( e->lhs, global, local, ifd, back_inserter( lhs ) );
			evalExpr( e->rhs, global, local, ifd, back_inserter( rhs ) );
			for( auto i = lhs.cbegin(); i != lhs.cend(); ++i ) {
				for( auto j = rhs.cbegin(); j != rhs.cend(); ++j ) {
					*dst++ = *i + *j;
				}
			}
		}
		VCASE( Var, e ) {
			auto& val = findVariable( e->name, local );
			dst = copy( val.begin(), val.end(), dst );
		}
		VCASE( Subst, e ) {
			int fds[2];
			checkSysCall( pipe( fds ) );

			auto reader = [&]() -> void {
				auto closer = scopeExit( bind( close, fds[0] ) );
				UnixIStream ifs( fds[0] );
				string buf;
				while( getline( ifs, buf ) ) {
					*dst++ = buf;
				}
			};
			auto writer = [&]() -> void {
				try {
					auto closer = scopeExit( bind( close, fds[1] ) );
					evalStmt( e->body, global, local, ifd, fds[1] );
				}
				catch( BreakException const& ) {
				}
				catch( ReturnException const& ) {
				}
			};
			parallel( writer, reader );
		}
		VCASE( Slice, e ) {
			deque<MetaString> sBgn;
			deque<MetaString> sEnd;
			evalExpr( e->bgn, global, local, ifd, back_inserter( sBgn ) );
			evalExpr( e->end, global, local, ifd, back_inserter( sEnd ) );
			if( sBgn.size() != 1 || sEnd.size() != 1 ) {
				// XXX
				return dst;
			}
			int bgn = readValue<int>( string( sBgn.back().begin(), sBgn.back().end() ) );
			int end = readValue<int>( string( sEnd.back().begin(), sEnd.back().end() ) );

			auto& val = findVariable( e->var->name, local );
			bgn = imod( bgn, val.size() );
			end = imod( end, val.size() );
			if( bgn < end ) {
				dst = copy( val.begin() + bgn, val.begin() + end, dst );
			}
			else {
				dst = copy( val.begin() + bgn, val.end(), dst );
				dst = copy( val.begin(), val.begin() + end, dst );
			}
		}
		VCASE( Index, e ) {
			deque<MetaString> sIdx;
			evalExpr( e->idx, global, local, ifd, back_inserter( sIdx ) );
			if( sIdx.size() != 1 ) {
				// XXX
				return dst;
			}
			int idx = readValue<int>( string( sIdx.back().begin(), sIdx.back().end() ) );

			auto& val = findVariable( e->var->name, local );
			idx = imod( idx, val.size() );
			*dst++ = val[idx];
		}
		VCASE( Null, _ ) {
		}
		VDEFAULT {
			assert( false );
		}
	}
	return dst;
}

int execCommand( deque<string>& args, Global* global, int ifd, int ofd ) {
	auto fit = global->funs.find( args[0] );
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

		for( auto it = local.defs.rbegin(); it != local.defs.rend(); ++it ) {
			execCommand( *it, global, ifd, ofd );
		}

		return retv;
	}

	auto bit = global->builtins.find( args[0] );
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

int evalStmt( ast::Stmt* stmt, Global* global, Local* local, int ifd, int ofd ) {
	using namespace ast;

	Thread::checkIntr();

	//try {
	VSWITCH( stmt ) {
		VCASE( Sequence, s ) {
			evalStmt( s->lhs, global, local, ifd, ofd );
			return evalStmt( s->rhs, global, local, ifd, ofd );
		}
		VCASE( Parallel, s ) {
			bool lret = false;
			bool rret = false;
			int lval = 0;
			int rval = 0;
			auto evalLhs = [&]() -> void {
				try {
					lval = evalStmt( s->lhs, global, local, ifd, ofd );
				}
				catch( ReturnException const& e ) {
					lret = true;
					lval = e.retv;
				}
			};
			auto evalRhs = [&]() -> void {
				try {
					rval = evalStmt( s->rhs, global, local, ifd, ofd );
				}
				catch( ReturnException const& e ) {
					lret = true;
					rval = e.retv;
				}
			};
			parallel( evalLhs, evalRhs );
			if( lret ) {
				throw ReturnException( lval );
			}
			if( rret ) {
				throw ReturnException( rval );
			}
			return lval || rval;
		}
		VCASE( Bg, s ) {
			Thread thread( bind( evalStmt, s->body, global, local, ifd, ofd ) );
			thread.detach();
			return 0;
		}
		VCASE( RedirFr, s ) {
			deque<string> args;
			evalArgs( s->file, global, local, ifd, back_inserter( args ) );
			int fd = open( args.back().c_str(), O_RDONLY );
			checkSysCall( fd );
			auto closer = scopeExit( bind( close, fd ) );
			return evalStmt( s->body, global, local, fd, ofd );
		}
		VCASE( RedirTo, s ) {
			deque<string> args;
			evalArgs( s->file, global, local, ifd, back_inserter( args ) );
			int fd = open( args.back().c_str(), O_WRONLY | O_CREAT, 0644 );
			checkSysCall( fd );
			auto closer = scopeExit( bind( close, fd ) );
			return evalStmt( s->body, global, local, ifd, fd );
		}
		VCASE( Command, s ) {
			deque<string> args;
			evalArgs( s->args, global, local, ifd, back_inserter( args ) );
			if( args.size() == 0 ) {
				return 0;
			}

			return execCommand( args, global, ifd, ofd );
		}
		VCASE( Return, s ) {
			deque<string> args;
			evalArgs( s->retv, global, local, ifd, back_inserter( args ) );
			if( args.size() != 1 ) {
				return 1;
			}
			throw ReturnException( readValue<int>( args[0] ) ) ;
		}
		VCASE( Fun, s ) {
			deque<string> args;
			evalArgs( s->name, global, local, ifd, back_inserter( args ) );
			if( args.size() != 1 ) {
				return 1;
			}
			global->funs[args[0]] = s;
			return 0;
		}
		VCASE( If, s ) {
			if( evalStmt( s->cond, global, local, ifd, ofd ) == 0 ) {
				return evalStmt( s->then, global, local, ifd, ofd );
			}
			else {
				return evalStmt( s->elze, global, local, ifd, ofd );
			}
		}
		VCASE( While, s ) {
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
		VCASE( Break, s ) {
			deque<string> args;
			evalArgs( s->retv, global, local, ifd, back_inserter( args ) );
			if( args.size() != 1 ) {
				return 1;
			}
			int retv = readValue<int>( args[0] );
			throw BreakException( retv );
		}
		VCASE( Let, s ) {
			deque<string> vals;
			evalArgs( s->rhs, global, local, ifd, back_inserter( vals ) );
			return assign( s->lhs, vals, local ) ? 0 : 1;
		}
		VCASE( Fetch, s ) {
			VSWITCH( s->lhs ) {
				VCASE( VarFix, lhs ) {
					UnixIStream ifs( ifd, 1 );
					deque<string> rhs( lhs->var.size() );
					for( auto it = rhs.begin(); it != rhs.end(); ++it ) {
						if( !getline( ifs, *it ) ) {
							return 1;
						}
					}

					return assign( lhs, rhs, local ) ? 0 : 1;
				}
				VCASE( VarVar, lhs ) {
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
				VDEFAULT {
					assert( false );
				}
			}
		}
		VCASE( Yield, s ) {
			deque<string> vals;
			evalArgs( s->rhs, global, local, ifd, back_inserter( vals ) );
			ostringstream buf;
			for( auto it = vals.cbegin(); it != vals.cend(); ++it ) {
				buf << *it << '\n';
			}
			writeAll( ofd, buf.str() );
			return 0;
		}
		VCASE( Pipe, s ) {
			int fds[2];
			checkSysCall( pipe( fds ) );

			bool lret = false;
			bool rret = false;
			int lval = 0;
			int rval = 0;
			auto evalLhs = [&]() -> void {
				try {
					auto closer = scopeExit( bind( close, fds[1] ) );
					lval = evalStmt( s->lhs, global, local, ifd, fds[1] );
				}
				catch( ReturnException const& e ) {
					lret = true;
					lval = e.retv;
				}
			};
			auto evalRhs = [&]() -> void {
				try {
					auto closer = scopeExit( bind( close, fds[0] ) );
					rval = evalStmt( s->rhs, global, local, fds[0], ofd );
				}
				catch( ReturnException const& e ) {
					rret = true;
					rval = e.retv;
				}
			};
			parallel( evalLhs, evalRhs );
			if( lret ) {
				throw ReturnException( lval );
			}
			if( rret ) {
				throw ReturnException( rval );
			}
			return lval || rval;
		}
		VCASE( Defer, s ) {
			local->defs.push_back( deque<string>() );
			evalArgs( s->args, global, local, ifd, back_inserter( local->defs.back() ) );
			return 0;
		}
		VCASE( None, s ) {
			return s->retv;
		}
		VDEFAULT {
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
