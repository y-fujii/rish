// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <algorithm>
#include <cassert>
#include <deque>
#include <exception>
#include <functional>
#include <iterator>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include "ast.hpp"
#include "glob.hpp"
#include "misc.hpp"
#include "unix.hpp"

using namespace std;


using Builtin = function<int (deque<string> const&, int, int)>;

struct ThreadComparator {
	bool operator()( thread const& x, thread const& y ) const {
		return const_cast<thread&>( x ).native_handle() < const_cast<thread&>( y ).native_handle();
	}
};

struct Local {
	Local( shared_ptr<Local> o = nullptr ): outer( o ) {}

	shared_ptr<Local> outer;
	map<string, deque<string>> vars;
	deque<deque<string>> defs;
	//pthread_mutex_t lock;
};

struct Global {
	struct Closure {
		ast::Fun* fun;
		shared_ptr<Local> env;
	};

	map<string, Closure> funs;
	map<string, Builtin> builtins;
	set<thread, ThreadComparator> fgThreads;
	set<thread, ThreadComparator> bgThreads;
	//map<string, pair<ast::Fun*, Local*> > funs;
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

int evalStmt( ast::Stmt*, Global&, shared_ptr<Local>, int, int );


inline deque<string>& findVariable( ast::Var const* var, Local& local ) {
	Local* it = &local;
	while( it != nullptr ) {
		auto v = it->vars.find( var->name );
		if( v != it->vars.end() ) {
			return v->second;
		}
		it = it->outer.get();
	}

	// new variable
	return local.vars[var->name];
}

// XXX
template<class Container>
bool assign( ast::VarFix* lhs, Container& rhs, Local& local ) {
	using namespace ast;

	if( rhs.size() != lhs->var.size() ) {
		return false;
	}

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
			VCASE( Var, var ) {
				auto& val = findVariable( var, local );
				val.clear();
				val.push_back( rhs[i] );
			}
			VDEFAULT {
			}
		}
	}

	return true;
}

// XXX
template<class Container>
bool assign( ast::VarVar* lhs, Container& rhs, Local& local ) {
	using namespace ast;

	if( rhs.size() < lhs->varL.size() + lhs->varR.size() ) {
		return false;
	}

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
			VCASE( Var, var ) {
				auto& val = findVariable( var, local );
				val.clear();
				val.push_back( rhs[i + lBgn] );
			}
			VDEFAULT {
			}
		}
	}
	auto& val = findVariable( lhs->varM, local );
	val.clear();
	copy( &rhs[mBgn], &rhs[rBgn], back_inserter( val ) );
	for( size_t i = 0; i < lhs->varR.size(); ++i ) {
		VSWITCH( lhs->varR[i] ) {
			VCASE( Var, var ) {
				auto& val = findVariable( var, local );
				val.clear();
				val.push_back( rhs[i + rBgn] );
			}
			VDEFAULT {
			}
		}
	}

	return true;
}

template<class Container>
bool assign( ast::LeftExpr* lhsb, Container& rhs, Local& local ) {
	using namespace ast;

	VSWITCH( lhsb ) {
		VCASE( VarFix, lhs ) {
			return assign( lhs, rhs, local );
		}
		VCASE( VarVar, lhs ) {
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
DstIter evalExpr( ast::Expr* expr, Global& global, shared_ptr<Local> local, int ifd, DstIter dst ) {
	using namespace ast;

	ThreadSupport::checkIntr();

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
			auto& val = findVariable( e, *local );
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

			auto& val = findVariable( e->var, *local );
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

			auto& val = findVariable( e->var, *local );
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

int execCommand( deque<string>& args, Global& global, int ifd, int ofd ) {
	auto fit = global.funs.find( args[0] );
	if( fit != global.funs.end() ) {
		auto local = make_shared<Local>( fit->second.env );
		int retv;
		try {
			args.pop_front();
			if( assign( fit->second.fun->args, args, *local ) ) {
				retv = evalStmt( fit->second.fun->body, global, local, ifd, ofd );
			}
			else {
				return 1; // to be implemented
			}
		}
		catch( ReturnException const& e ) {
			retv = e.retv;
		}

		for( auto it = local->defs.rbegin(); it != local->defs.rend(); ++it ) {
			execCommand( *it, global, ifd, ofd );
		}

		return retv;
	}

	auto bit = global.builtins.find( args[0] );
	if( bit != global.builtins.end() ) {
		args.pop_front();
		return bit->second( args, ifd, ofd );
	}

	pid_t pid = forkExec( args, ifd, ofd );
	int status;
	checkSysCall( waitpid( pid, &status, 0 ) );
	return WEXITSTATUS( status );
}

template<class DstIter>
DstIter evalArgs( ast::Expr* expr, Global& global, shared_ptr<Local> local, int ifd, DstIter dstIt ) {
	struct Inserter: std::iterator<output_iterator_tag, Inserter> {
		DstIter dstIt;

		Inserter( DstIter it ): dstIt( it ) {
		}
		Inserter& operator*() {
			return *this;
		}
		Inserter& operator++() {
			return *this;
		}
		Inserter& operator++( int ) {
			return *this;
		}
		Inserter operator=( MetaString const& str ) {
			this->dstIt = expandGlob( str, this->dstIt );
			return *this;
		}
	};
	Inserter inserter( dstIt );
	evalExpr( expr, global, local, ifd, inserter );
	return inserter.dstIt;
}

int evalStmt( ast::Stmt* stmt, Global& global, shared_ptr<Local> local, int ifd, int ofd ) {
	using namespace ast;

	ThreadSupport::checkIntr();

	try { VSWITCH( stmt ) {
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
			// XXX: stdin, stdout
			thread thr( evalStmt, s->body, ref( global ), local, 0, 1 );
			writeAll( ofd, 'T' + ThreadSupport::name( thr ) + '\n' );
			global.bgThreads.insert( move( thr ) );
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
			global.funs[args[0]] = { s, local };
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
			return assign( s->lhs, vals, *local ) ? 0 : 1;
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
					return assign( lhs, rhs, *local ) ? 0 : 1;
				}
				VCASE( VarVar, lhs ) {
					deque<string> rhs;
					UnixIStream ifs( ifd );
					string buf;
					while( getline( ifs, buf ) ) {
						rhs.push_back( buf );
					}
					return assign( lhs, rhs, *local ) ? 0 : 1;
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
	} }
	catch( IOError const& ) {
		return 1;
	}

	assert( false );
	return -1;
}
