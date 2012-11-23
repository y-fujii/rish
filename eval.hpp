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


struct Evaluator {
	Evaluator(): stdin( 0 ), stdout( 1 ), stderr( 2 ) {} // XXX

	using Builtin = function<int (deque<string> const&, Evaluator&, int, int)>;

	struct Local {
		deque<string>& value( ast::Var* );
		template<class Container> bool assign( ast::VarFix*, Container&& );
		template<class Container> bool assign( ast::VarVar*, Container&& );
		template<class Container> bool assign( ast::LeftExpr*, Container&& );

		shared_ptr<Local> outer;
		map<string, deque<string>> vars;
		deque<deque<string>> defs;
	};

	struct Closure {
		ast::Fun* fun;
		shared_ptr<Local> env;
	};

	struct BreakException {
		BreakException( int r ): retv( r ) {}
		int const retv;
	};

	struct ReturnException {
		ReturnException( int r ): retv( r ) {}
		int const retv;
	};

	int execCommand( deque<string>&&, int, int );
	template<class DstIter> DstIter evalExpr( ast::Expr*, shared_ptr<Local>, int, DstIter );
	template<class DstIter> DstIter evalArgs( ast::Expr*, shared_ptr<Local>, int, DstIter );
	int evalStmt( ast::Stmt*, shared_ptr<Local>, int, int );
	int evalStmt( ast::Stmt*, shared_ptr<Local> ); // XXX

	map<string, Closure> closures;
	map<string, Builtin> builtins;
	map<thread::id, thread> foregrounds;
	map<thread::id, thread> backgrounds;
	mutex mutexGlobal;

	int stdin;
	int stdout;
	int stderr;
};


inline deque<string>& Evaluator::Local::value( ast::Var* var ) {
	Local* it = this;
	while( it != nullptr ) {
		auto v = it->vars.find( var->name );
		if( v != it->vars.end() ) {
			return v->second;
		}
		it = it->outer.get();
	}

	// new variable
	return vars[var->name];
}

// XXX
template<class Container>
bool Evaluator::Local::assign( ast::VarFix* lhs, Container&& rhs ) {
	using namespace ast;
	static_assert( !std::is_lvalue_reference<Container>::value, "" );

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

	for( size_t i = 0; i < rhs.size(); ++i ) {
		VSWITCH( lhs->var[i] ) {
			VCASE( Var, var ) {
				auto& val = value( var );
				val.clear();
				val.push_back( move( rhs[i] ) );
			}
			VDEFAULT {
			}
		}
	}

	return true;
}

// XXX
template<class Container>
bool Evaluator::Local::assign( ast::VarVar* lhs, Container&& rhs ) {
	using namespace ast;
	static_assert( !std::is_lvalue_reference<Container>::value, "" );

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

	for( size_t i = 0; i < lhs->varL.size(); ++i ) {
		VSWITCH( lhs->varL[i] ) {
			VCASE( Var, var ) {
				auto& val = value( var );
				val.clear();
				val.push_back( move( rhs[i + lBgn] ) );
			}
			VDEFAULT {
			}
		}
	}
	auto& val = value( lhs->varM );
	val.clear();
	move( &rhs[mBgn], &rhs[rBgn], back_inserter( val ) );
	for( size_t i = 0; i < lhs->varR.size(); ++i ) {
		VSWITCH( lhs->varR[i] ) {
			VCASE( Var, var ) {
				auto& val = value( var );
				val.clear();
				val.push_back( move( rhs[i + rBgn] ) );
			}
			VDEFAULT {
			}
		}
	}

	return true;
}

template<class Container>
bool Evaluator::Local::assign( ast::LeftExpr* lhsb, Container&& rhs ) {
	using namespace ast;

	VSWITCH( lhsb ) {
		VCASE( VarFix, lhs ) {
			return assign( lhs, forward<Container>( rhs ) );
		}
		VCASE( VarVar, lhs ) {
			return assign( lhs, forward<Container>( rhs ) );
		}
		VDEFAULT {
			assert( false );
		}
	}

	assert( false );
	return false;
}

template<class DstIter>
DstIter Evaluator::evalExpr( ast::Expr* expr, shared_ptr<Local> local, int ifd, DstIter dst ) {
	using namespace ast;

	ThreadSupport::checkIntr();

	VSWITCH( expr ) {
		VCASE( Word, e ) {
			*dst++ = e->word;
		}
		VCASE( Pair, e ) {
			evalExpr( e->lhs, local, ifd, dst );
			evalExpr( e->rhs, local, ifd, dst );
		}
		VCASE( Concat, e ) {
			deque<MetaString> lhs;
			deque<MetaString> rhs;
			evalExpr( e->lhs, local, ifd, back_inserter( lhs ) );
			evalExpr( e->rhs, local, ifd, back_inserter( rhs ) );
			for( auto i = lhs.cbegin(); i != lhs.cend(); ++i ) {
				for( auto j = rhs.cbegin(); j != rhs.cend(); ++j ) {
					*dst++ = *i + *j;
				}
			}
		}
		VCASE( Var, e ) {
			lock_guard<mutex> lock( mutexGlobal );
			auto& val = local->value( e );
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
					evalStmt( e->body, local, ifd, fds[1] );
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
			evalExpr( e->bgn, local, ifd, back_inserter( sBgn ) );
			evalExpr( e->end, local, ifd, back_inserter( sEnd ) );
			if( sBgn.size() != 1 || sEnd.size() != 1 ) {
				// XXX
				return dst;
			}
			int bgn = readValue<int>( string( sBgn.back().begin(), sBgn.back().end() ) );
			int end = readValue<int>( string( sEnd.back().begin(), sEnd.back().end() ) );

			lock_guard<mutex> lock( mutexGlobal );
			auto& val = local->value( e->var );
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
			evalExpr( e->idx, local, ifd, back_inserter( sIdx ) );
			if( sIdx.size() != 1 ) {
				// XXX
				return dst;
			}
			int idx = readValue<int>( string( sIdx.back().begin(), sIdx.back().end() ) );

			lock_guard<mutex> lock( mutexGlobal );
			auto& val = local->value( e->var );
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

inline int Evaluator::execCommand( deque<string>&& args, int ifd, int ofd ) {
	assert( args.size() >= 1 );

	mutexGlobal.lock(); // XXX
	auto fit = closures.find( args[0] );
	if( fit != closures.end() ) {
		ast::Fun* fun = fit->second.fun;
		shared_ptr<Local> env = fit->second.env;
		mutexGlobal.unlock();

		args.pop_front();
		auto local = make_shared<Local>();
		if( !local->assign( fun->args, move( args ) ) ) {
			return 1; // to be implemented
		}
		local->outer = move( env );

		int retv;
		try {
			retv = evalStmt( fun->body, local, ifd, ofd );
		}
		catch( ReturnException const& e ) {
			retv = e.retv;
		}

		for( auto it = local->defs.rbegin(); it != local->defs.rend(); ++it ) {
			execCommand( move( *it ), ifd, ofd );
		}
		// loca.defs is not required anymore but local itself may be referenced
		// by other closures.
		local->defs = deque<deque<string>>();

		return retv;
	}
	mutexGlobal.unlock();

	auto bit = builtins.find( args[0] );
	if( bit != builtins.end() ) {
		args.pop_front();
		return bit->second( args, *this, ifd, ofd );
	}

	pid_t pid = forkExec( args, ifd, ofd );
	int status;
	checkSysCall( waitpid( pid, &status, 0 ) );
	return WEXITSTATUS( status );
}

template<class DstIter>
DstIter Evaluator::evalArgs( ast::Expr* expr, shared_ptr<Local> local, int ifd, DstIter dstIt ) {
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
	evalExpr( expr, local, ifd, inserter );
	return inserter.dstIt;
}

inline int Evaluator::evalStmt( ast::Stmt* stmt, shared_ptr<Local> local, int ifd, int ofd ) {
	using namespace ast;

	ThreadSupport::checkIntr();

	try { VSWITCH( stmt ) {
		VCASE( Sequence, s ) {
			evalStmt( s->lhs, local, ifd, ofd );
			return evalStmt( s->rhs, local, ifd, ofd );
		}
		VCASE( Parallel, s ) {
			bool lret = false;
			bool rret = false;
			int lval = 0;
			int rval = 0;
			auto evalLhs = [&]() -> void {
				try {
					lval = evalStmt( s->lhs, local, ifd, ofd );
				}
				catch( ReturnException const& e ) {
					lret = true;
					lval = e.retv;
				}
			};
			auto evalRhs = [&]() -> void {
				try {
					rval = evalStmt( s->rhs, local, ifd, ofd );
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
			Stmt* body = s->body;
			thread thr( [=]() -> void {
				this->evalStmt( body, local, this->stdin, this->stdout );
			} );
			thread::id id = thr.get_id();
			{
				lock_guard<mutex> lock( mutexGlobal );
				backgrounds[id] = move( thr );
			}

			ostringstream ofs;
			ofs.exceptions( ios_base::failbit | ios_base::badbit );
			ofs << id << '\n';
			writeAll( ofd, ofs.str() );
			return 0;
		}
		VCASE( RedirFr, s ) {
			deque<string> args;
			evalArgs( s->file, local, ifd, back_inserter( args ) );
			int fd = open( args.back().c_str(), O_RDONLY );
			checkSysCall( fd );
			auto closer = scopeExit( bind( close, fd ) );
			return evalStmt( s->body, local, fd, ofd );
		}
		VCASE( RedirTo, s ) {
			deque<string> args;
			evalArgs( s->file, local, ifd, back_inserter( args ) );
			int fd = open( args.back().c_str(), O_WRONLY | O_CREAT, 0644 );
			checkSysCall( fd );
			auto closer = scopeExit( bind( close, fd ) );
			return evalStmt( s->body, local, ifd, fd );
		}
		VCASE( Command, s ) {
			deque<string> args;
			evalArgs( s->args, local, ifd, back_inserter( args ) );
			if( args.size() == 0 ) {
				return 0;
			}

			return execCommand( move( args ), ifd, ofd );
		}
		VCASE( Return, s ) {
			deque<string> args;
			evalArgs( s->retv, local, ifd, back_inserter( args ) );
			if( args.size() != 1 ) {
				return 1;
			}
			throw ReturnException( readValue<int>( args[0] ) ) ;
		}
		VCASE( Fun, s ) {
			deque<string> args;
			evalArgs( s->name, local, ifd, back_inserter( args ) );
			if( args.size() != 1 ) {
				return 1;
			}

			lock_guard<mutex> lock( mutexGlobal );
			closures[args[0]] = { s, local };
			return 0;
		}
		VCASE( FunDel, s ) {
			deque<string> args;
			evalArgs( s->name, local, ifd, back_inserter( args ) );
			if( args.size() != 1 ) {
				return 1;
			}

			lock_guard<mutex> lock( mutexGlobal );
			return closures.erase( args[0] ) != 0 ? 0 : 1;
		}
		VCASE( If, s ) {
			if( evalStmt( s->cond, local, ifd, ofd ) == 0 ) {
				return evalStmt( s->then, local, ifd, ofd );
			}
			else {
				return evalStmt( s->elze, local, ifd, ofd );
			}
		}
		VCASE( While, s ) {
			try {
				while( evalStmt( s->cond, local, ifd, ofd ) == 0 ) {
					evalStmt( s->body, local, ifd, ofd );
				}
			}
			catch( BreakException const& e ) {
				return e.retv;
			}
			return evalStmt( s->elze, local, ifd, ofd );
		}
		VCASE( Break, s ) {
			deque<string> args;
			evalArgs( s->retv, local, ifd, back_inserter( args ) );
			if( args.size() != 1 ) {
				return 1;
			}
			int retv = readValue<int>( args[0] );
			throw BreakException( retv );
		}
		VCASE( Let, s ) {
			deque<string> vals;
			evalArgs( s->rhs, local, ifd, back_inserter( vals ) );

			lock_guard<mutex> lock( mutexGlobal );
			return local->assign( s->lhs, move( vals ) ) ? 0 : 1;
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

					lock_guard<mutex> lock( mutexGlobal );
					return local->assign( lhs, move( rhs ) ) ? 0 : 1;
				}
				VCASE( VarVar, lhs ) {
					deque<string> rhs;
					UnixIStream ifs( ifd );
					string buf;
					while( getline( ifs, buf ) ) {
						rhs.push_back( buf );
					}

					lock_guard<mutex> lock( mutexGlobal );
					return local->assign( lhs, move( rhs ) ) ? 0 : 1;
				}
				VDEFAULT {
					assert( false );
				}
			}
		}
		VCASE( Yield, s ) {
			deque<string> vals;
			evalArgs( s->rhs, local, ifd, back_inserter( vals ) );
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
					lval = evalStmt( s->lhs, local, ifd, fds[1] );
				}
				catch( ReturnException const& e ) {
					lret = true;
					lval = e.retv;
				}
			};
			auto evalRhs = [&]() -> void {
				try {
					auto closer = scopeExit( bind( close, fds[0] ) );
					rval = evalStmt( s->rhs, local, fds[0], ofd );
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
			deque<string> args;
			evalArgs( s->args, local, ifd, back_inserter( args ) );

			lock_guard<mutex> lock( mutexGlobal );
			local->defs.push_back( move( args ) );
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

inline int Evaluator::evalStmt( ast::Stmt* stmt, shared_ptr<Local> local ) {
	return evalStmt( stmt, local, stdin, stdout );
}
