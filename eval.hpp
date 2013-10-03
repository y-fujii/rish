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
#include <system_error>
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
	Evaluator(): separator( '\n' ) {} // XXX

	struct Local {
		deque<string>& value( ast::Var* );
		template<class Iter> bool assign( ast::VarFix*, Iter, Iter );
		template<class Iter> bool assign( ast::VarVar*, Iter, Iter );
		template<class Iter> bool assign( ast::LeftExpr*, Iter, Iter );

		shared_ptr<Local> outer;
		deque<deque<string>> vars;
		deque<deque<string>> defs;
		string cwd;
	};

	struct Closure {
		int nVar;
		shared_ptr<ast::LeftExpr> args;
		shared_ptr<ast::Stmt> body;
		shared_ptr<Local> env;
	};

	struct BreakException {
		explicit BreakException( int r ): retv( r ) {}
		int const retv;
	};

	struct ReturnException {
		explicit ReturnException( int r ): retv( r ) {}
		int const retv;
	};

	using ArgIter = move_iterator<deque<string>::iterator>;
	using Builtin = function<int ( ArgIter, ArgIter, Evaluator&, Local const&, int, int )>;

	template<class Iter> int callCommand( Iter, Iter, Local const&, int, int );
	template<class Iter> Iter evalExpr( ast::Expr*, shared_ptr<Local>, Iter );
	template<class Iter> Iter evalArgs( ast::Expr*, shared_ptr<Local>, Iter );
	int evalStmt( ast::Stmt*, shared_ptr<Local>, int, int );
	future<int> evaluate( ast::Stmt*, shared_ptr<Local>, int, int );
	void join();
	void interrupt();

	map<string, Closure> closures;
	map<string, Builtin> builtins;
	map<thread::id, thread> foregrounds;
	map<thread::id, thread> backgrounds;
	mutex mutexGlobal;

	char separator; // XXX: better to be function local?
};


inline deque<string>& Evaluator::Local::value( ast::Var* var ) {
	assert( var->depth >= 0 );
	assert( var->index >= 0 );

	Local* it = this;
	for( int i = 0; i < var->depth; ++i ) {
		it = it->outer.get();
	}
	return it->vars[var->index];
}

template<class Iter>
bool Evaluator::Local::assign( ast::VarFix* lhs, Iter rhsB, Iter rhsE ) {
	using namespace ast;

	if( lhs->var.size() != size_t( rhsE - rhsB ) ) {
		return false;
	}

	// test
	for( size_t i = 0; i < lhs->var.size(); ++i ) {
		auto word = match<Word>( lhs->var[i].get() );
		if( word && word->word != MetaString( rhsB[i] ) ) {
			return false;
		}
	}

	// assign
	for( size_t i = 0; i < lhs->var.size(); ++i ) {
		if( auto var = match<Var>( lhs->var[i].get() ) ) {
			value( var ) = { rhsB[i] };
		}
	}

	return true;
}

template<class Iter>
bool Evaluator::Local::assign( ast::VarVar* lhs, Iter rhsB, Iter rhsE ) {
	using namespace ast;

	if( lhs->varL.size() + lhs->varR.size() > size_t( rhsE - rhsB ) ) {
		return false;
	}

	Iter rhsL = rhsB;
	Iter rhsM = rhsB + lhs->varL.size();
	Iter rhsR = rhsE - lhs->varR.size();

	// test varL
	for( size_t i = 0; i < lhs->varL.size(); ++i ) {
		auto word = match<Word>( lhs->varL[i].get() );
		if( word && word->word != MetaString( rhsL[i] ) ) {
			return false;
		}
	}

	// test varR
	for( size_t i = 0; i < lhs->varR.size(); ++i ) {
		auto word = match<Word>( lhs->varR[i].get() );
		if( word && word->word != MetaString( rhsR[i] ) ) {
			return false;
		}
	}

	// assign varL
	for( size_t i = 0; i < lhs->varL.size(); ++i ) {
		if( auto var = match<Var>( lhs->varL[i].get() ) ) {
			value( var ) = { rhsL[i] };
		}
	}

	// assign varM
	auto& val = value( lhs->varM.get() );
	val.resize( rhsR - rhsM );
	copy( rhsM, rhsR, val.begin() );

	// assign varR
	for( size_t i = 0; i < lhs->varR.size(); ++i ) {
		if( auto var = match<Var>( lhs->varR[i].get() ) ) {
			value( var ) = { rhsR[i] };
		}
	}

	return true;
}

template<class Iter>
bool Evaluator::Local::assign( ast::LeftExpr* lhs, Iter rhsB, Iter rhsE ) {
	using namespace ast;

	VSWITCH( lhs ) {
		VCASE( VarFix, lhs ) {
			return assign( lhs, rhsB, rhsE );
		}
		VCASE( VarVar, lhs ) {
			return assign( lhs, rhsB, rhsE );
		}
		VDEFAULT {
			assert( false );
		}
	}

	assert( false );
	return false;
}

template<class DstIter>
DstIter Evaluator::evalExpr( ast::Expr* expr, shared_ptr<Local> local, DstIter dst ) {
	using namespace ast;

tailRec:
	VSWITCH( expr ) {
		VCASE( Word, e ) {
			*dst++ = e->word;
		}
		VCASE( Pair, e ) {
			evalExpr( e->lhs.get(), local, dst );

			// evalExpr( e->rhs.get(), local, dst );
			expr = e->rhs.get();
			goto tailRec;
		}
		VCASE( Concat, e ) {
			deque<MetaString> lhs;
			deque<MetaString> rhs;
			evalExpr( e->lhs.get(), local, back_inserter( lhs ) );
			evalExpr( e->rhs.get(), local, back_inserter( rhs ) );
			for( auto const& lv: lhs ) {
				for( auto const& rv: rhs ) {
					*dst++ = lv + rv;
				}
			}
		}
		VCASE( Var, e ) {
			lock_guard<mutex> lock( mutexGlobal );
			auto& val = local->value( e );
			dst = copy( val.cbegin(), val.cend(), dst );
		}
		VCASE( Subst, e ) {
			int fds[2];
			checkSysCall( pipe( fds ) );

			auto reader = [&]() -> void {
				auto closer = scopeExit( bind( close, fds[0] ) );
				UnixIStream<> ifs( fds[0] );
				string buf;
				while( getline( ifs, buf, separator ) ) {
					*dst++ = buf;
				}
			};
			auto writer = [&]() -> void {
				try {
					auto ocloser = scopeExit( bind( close, fds[1] ) );
					int ifd = checkSysCall( open( "/dev/null", O_RDONLY ) );
					auto icloser = scopeExit( bind( close, ifd ) );

					evalStmt( e->body.get(), local, ifd, fds[1] );
				}
				catch( BreakException const& ) {
				}
				catch( ReturnException const& ) {
				}
			};
			parallel( writer, reader );
		}
		VCASE( BinOp, e ) {
			deque<MetaString> lhss, rhss;
			evalExpr( e->lhs.get(), local, back_inserter( lhss ) );
			evalExpr( e->rhs.get(), local, back_inserter( rhss ) );
			if( (lhss.size() != 0 && rhss.size() == 0) ||
			    (lhss.size() == 0 && rhss.size() != 0) ) {
				throw invalid_argument( "" );
			}

			for( size_t i = 0; i < lhss.size() || i < rhss.size(); ++i ) {
				MetaString const& lhs = lhss[i % lhss.size()];
				MetaString const& rhs = rhss[i % rhss.size()];
				int64_t lval = stoll( string( lhs.cbegin(), lhs.cend() ) );
				int64_t rval = stoll( string( rhs.cbegin(), rhs.cend() ) );
				int64_t r;
				switch( e->op ) {
					case BinOp::add: r = lval + rval; break;
					case BinOp::sub: r = lval - rval; break;
					case BinOp::mul: r = lval * rval; break;
					case BinOp::div:
						if( rval == 0 ) {
							throw invalid_argument( "" );
						}
						r = idiv( lval, rval );
						break;
					case BinOp::mod:
						if( rval == 0 ) {
							throw invalid_argument( "" );
						}
						r = imod( lval, rval );
						break;
					case BinOp::eq: r = (lval == rval) ? 0 : -1; break;
					case BinOp::ne: r = (lval != rval) ? 0 : -1; break;
					case BinOp::le: r = (lval <= rval) ? 0 : -1; break;
					case BinOp::ge: r = (lval >= rval) ? 0 : -1; break;
					case BinOp::lt: r = (lval <  rval) ? 0 : -1; break;
					case BinOp::gt: r = (lval >  rval) ? 0 : -1; break;
				}
				*dst++ = to_string( r );
			}
		}
		VCASE( UniOp, e ) {
			deque<MetaString> lhs;
			evalExpr( e->lhs.get(), local, back_inserter( lhs ) );
			for( auto const& v: lhs ) {
				int64_t val = stoll( string( v.cbegin(), v.cend() ) );
				int64_t r;
				switch( e->op ) {
					case UniOp::pos: r = +val; break;
					case UniOp::neg: r = -val; break;
				}
				*dst++ = to_string( r );
			}
		}
		VCASE( Size, e ) {
			lock_guard<mutex> lock( mutexGlobal );
			auto& val = local->value( e->var.get() );
			*dst++ = to_string( val.size() );
		}
		VCASE( Index, e ) {
			deque<MetaString> sIdcs;
			evalExpr( e->idx.get(), local, back_inserter( sIdcs ) );

			lock_guard<mutex> lock( mutexGlobal );
			auto& val = local->value( e->var.get() );
			if( val.size() == 0 && sIdcs.size() != 0 ) {
				throw invalid_argument( "" );
			}
			for( auto const& sIdx: sIdcs ) {
				int64_t idx = stoll( string( sIdx.cbegin(), sIdx.cend() ) );
				idx = imod( idx, val.size() );
				*dst++ = val[idx];
			}
		}
		VCASE( Slice, e ) {
			deque<MetaString> sBgns;
			deque<MetaString> sEnds;
			evalExpr( e->bgn.get(), local, back_inserter( sBgns ) );
			evalExpr( e->end.get(), local, back_inserter( sEnds ) );
			if( (sBgns.size() != 0 && sEnds.size() == 0) ||
			    (sBgns.size() == 0 && sEnds.size() != 0) ) {
				throw invalid_argument( "" );
			}

			lock_guard<mutex> lock( mutexGlobal );
			auto& val = local->value( e->var.get() );
			if( val.size() == 0 && (sBgns.size() != 0 || sEnds.size() != 0) ) {
				throw invalid_argument( "" );
			}

			for( size_t i = 0; i < sBgns.size() || i < sEnds.size(); ++i ) {
				MetaString const& sBgn = sBgns[i % sBgns.size()];
				MetaString const& sEnd = sEnds[i % sEnds.size()];
				int64_t bgn = stoll( string( sBgn.cbegin(), sBgn.cend() ) );
				int64_t end = stoll( string( sEnd.cbegin(), sEnd.cend() ) );
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
		}
		VCASE( Null, _ ) {
		}
		VDEFAULT {
			assert( false );
		}
	}
	return dst;
}

template<class Iter>
int Evaluator::callCommand( Iter argsB, Iter argsE, Local const& local, int ifd, int ofd ) {
	assert( argsE - argsB >= 1 );

	if( argsE - argsB == 1 ) {
		try {
			return stoll( argsB[0] );
		}
		catch( invalid_argument const& ) {
		}
	}

	mutexGlobal.lock();
	auto fit = closures.find( argsB[0] );
	if( fit != closures.end() ) {
		Closure cl = fit->second;
		mutexGlobal.unlock();

		auto child = make_shared<Local>();
		child->vars.resize( cl.nVar );
		if( !child->assign( cl.args.get(), argsB + 1, argsE ) ) {
			throw invalid_argument( "" ); // or allow overloaded functions?
		}
		child->outer = move( cl.env );
		child->cwd = local.cwd;

		int retv;
		try {
			retv = evalStmt( cl.body.get(), child, ifd, ofd );
		}
		catch( ReturnException const& e ) {
			retv = e.retv;
		}

		for( auto it = child->defs.rbegin(); it != child->defs.rend(); ++it ) {
			callCommand(
				make_move_iterator( it->begin() ),
				make_move_iterator( it->end() ),
				*child, ifd, ofd
			);
		}
		// child.defs is not required anymore but local itself may be
		// referenced by other closures.
		child->defs = {};

		return retv;
	}
	else {
		mutexGlobal.unlock();
	}

	auto bit = builtins.find( argsB[0] );
	if( bit != builtins.end() ) {
		try {
			return bit->second( argsB + 1, argsE, *this, local, ifd, ofd );
		}
		catch( std::exception const& ) {
			return 1;
		}
	}

	pid_t pid = forkExec( argsB, argsE, ifd, ofd, local.cwd );
	int status;
	checkSysCall( waitpid( pid, &status, 0 ) );
	return WEXITSTATUS( status );
}

template<class DstIter>
DstIter Evaluator::evalArgs( ast::Expr* expr, shared_ptr<Local> local, DstIter dstIt ) {
	struct Inserter: std::iterator<output_iterator_tag, Inserter> {
		string const* cwd;
		DstIter dstIt;

		explicit Inserter( string const* c, DstIter it ):
			cwd( c ), dstIt( it ) {
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
			this->dstIt = expandGlob( str, *this->cwd, this->dstIt );
			return *this;
		}
	};
	Inserter inserter( &local->cwd, dstIt );
	evalExpr( expr, local, inserter );
	return inserter.dstIt;
}

inline int Evaluator::evalStmt( ast::Stmt* stmt, shared_ptr<Local> local, int ifd, int ofd ) {
	using namespace ast;

tailRec:
	ThreadSupport::checkIntr();

	try { VSWITCH( stmt ) {
		VCASE( Sequence, s ) {
			evalStmt( s->lhs.get(), local, ifd, ofd );

			// return evalStmt( s->rhs.get(), local, ifd, ofd );
			stmt = s->rhs.get();
			goto tailRec;
		}
		VCASE( Parallel, s ) {
			bool lret = false;
			bool rret = false;
			int lval = 0;
			int rval = 0;
			auto evalLhs = [&]() -> void {
				try {
					lval = evalStmt( s->lhs.get(), local, ifd, ofd );
				}
				catch( BreakException const& e ) {
					lval = e.retv;
				}
				catch( ReturnException const& e ) {
					lret = true;
					lval = e.retv;
				}
			};
			auto evalRhs = [&]() -> void {
				try {
					rval = evalStmt( s->rhs.get(), local, ifd, ofd );
				}
				catch( BreakException const& e ) {
					rval = e.retv;
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
			// keep the reference to AST
			shared_ptr<Stmt> body = s->body;
			thread thr( [=]() -> void {
				int ifd = checkSysCall( open( "/dev/null", O_RDONLY ) );
				auto icloser = scopeExit( bind( close, ifd ) );
				int ofd = checkSysCall( open( "/dev/null", O_WRONLY ) );
				auto ocloser = scopeExit( bind( close, ofd ) );

				this->evalStmt( body.get(), local, ifd, ofd );
			} );
			thread::id id = thr.get_id();
			{
				lock_guard<mutex> lock( mutexGlobal );
				backgrounds[id] = move( thr );
			}

			ostringstream ofs;
			ofs.exceptions( ios_base::failbit | ios_base::badbit );
			ofs << id << separator;
			writeAll( ofd, ofs.str() );
			return 0;
		}
		VCASE( RedirFr, s ) {
			deque<string> args;
			evalArgs( s->file.get(), local, back_inserter( args ) );
			if( args.size() != 1 ) {
				throw invalid_argument( "" );
			}

			int fd = open( args[0].c_str(), O_RDONLY );
			checkSysCall( fd );
			auto closer = scopeExit( bind( close, fd ) );
			return evalStmt( s->body.get(), local, fd, ofd );
		}
		VCASE( RedirTo, s ) {
			deque<string> args;
			evalArgs( s->file.get(), local, back_inserter( args ) );
			if( args.size() != 1 ) {
				throw invalid_argument( "" );
			}

			int fd = open( args[0].c_str(), O_WRONLY | O_CREAT, 0644 );
			checkSysCall( fd );
			auto closer = scopeExit( bind( close, fd ) );
			return evalStmt( s->body.get(), local, ifd, fd );
		}
		VCASE( Command, s ) {
			deque<string> args;
			evalArgs( s->args.get(), local, back_inserter( args ) );
			if( args.size() == 0 ) {
				return 0;
			}

			return callCommand(
				make_move_iterator( args.begin() ),
				make_move_iterator( args.end() ),
				*local, ifd, ofd
			);
		}
		VCASE( Return, s ) {
			deque<string> args;
			evalArgs( s->retv.get(), local, back_inserter( args ) );
			switch( args.size() ) {
				case 0:
					throw ReturnException( 0 );
				case 1:
					throw ReturnException( stoi( args[0] ) );
				default:
					throw invalid_argument( "" );
			}
		}
		VCASE( Fun, s ) {
			deque<string> args;
			evalArgs( s->name.get(), local, back_inserter( args ) );
			if( args.size() != 1 ) {
				throw invalid_argument( "" );
			}

			lock_guard<mutex> lock( mutexGlobal );
			closures[args[0]] = { s->nVar, s->args, s->body, local };
			return 0;
		}
		VCASE( FunDel, s ) {
			deque<string> args;
			evalArgs( s->name.get(), local, back_inserter( args ) );
			if( args.size() != 1 ) {
				throw invalid_argument( "" );
			}

			lock_guard<mutex> lock( mutexGlobal );
			return closures.erase( args[0] ) != 0 ? 0 : 1;
		}
		VCASE( If, s ) {
			if( evalStmt( s->cond.get(), local, ifd, ofd ) == 0 ) {
				// return evalStmt( s->then.get(), local, ifd, ofd );
				stmt = s->then.get();
				goto tailRec;
			}
			else {
				// return evalStmt( s->elze.get(), local, ifd, ofd );
				stmt = s->elze.get();
				goto tailRec;
			}
		}
		VCASE( While, s ) {
			while( evalStmt( s->cond.get(), local, ifd, ofd ) == 0 ) {
				try {
					evalStmt( s->body.get(), local, ifd, ofd );
				}
				catch( BreakException const& e ) {
					return e.retv;
				}
			}

			// return evalStmt( s->elze.get(), local, ifd, ofd );
			stmt = s->elze.get();
			goto tailRec;
		}
		VCASE( Break, s ) {
			deque<string> args;
			evalArgs( s->retv.get(), local, back_inserter( args ) );
			switch( args.size() ) {
				case 0:
					throw BreakException( 0 );
				case 1:
					throw BreakException( stoi( args[0] ) );
				default:
					throw invalid_argument( "" );
			}
		}
		VCASE( Let, s ) {
			deque<string> vals;
			evalArgs( s->rhs.get(), local, back_inserter( vals ) );

			lock_guard<mutex> lock( mutexGlobal );
			return local->assign(
				s->lhs.get(),
				make_move_iterator( vals.begin() ),
				make_move_iterator( vals.end() )
			) ? 0 : 1;
		}
		VCASE( Fetch, s ) {
			VSWITCH( s->lhs.get() ) {
				VCASE( VarFix, lhs ) {
					UnixIStream<1> ifs( ifd );
					deque<string> rhs( lhs->var.size() );
					for( auto& v: rhs ) {
						if( !getline( ifs, v, separator ) ) {
							return 1;
						}
					}

					lock_guard<mutex> lock( mutexGlobal );
					return local->assign(
						lhs,
						make_move_iterator( rhs.begin() ),
						make_move_iterator( rhs.end() )
					) ? 0 : 1;
				}
				VCASE( VarVar, lhs ) {
					deque<string> rhs;
					UnixIStream<> ifs( ifd );
					string buf;
					while( getline( ifs, buf, separator ) ) {
						rhs.push_back( buf );
					}

					lock_guard<mutex> lock( mutexGlobal );
					return local->assign(
						lhs,
						make_move_iterator( rhs.begin() ),
						make_move_iterator( rhs.end() )
					) ? 0 : 1;
				}
				VDEFAULT {
					assert( false );
				}
			}
		}
		VCASE( Yield, s ) {
			deque<string> vals;
			evalArgs( s->rhs.get(), local, back_inserter( vals ) );
			ostringstream buf;
			for( auto const& v: vals ) {
				buf << v << separator;
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
					lval = evalStmt( s->lhs.get(), local, ifd, fds[1] );
				}
				catch( BreakException const& e ) {
					lval = e.retv;
				}
				catch( ReturnException const& e ) {
					lret = true;
					lval = e.retv;
				}
			};
			auto evalRhs = [&]() -> void {
				try {
					auto closer = scopeExit( bind( close, fds[0] ) );
					rval = evalStmt( s->rhs.get(), local, fds[0], ofd );
				}
				catch( BreakException const& e ) {
					rval = e.retv;
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
		VCASE( Zip, s ) {
			if( s->exprs.size() == 0 ) {
				return 0;
			}

			deque<deque<string>> vals( s->exprs.size() );
			// evaluate all elements even if they have different sizes
			bool error = false;
			for( size_t i = 0; i < s->exprs.size(); ++i ) {
				evalArgs( s->exprs[i].get(), local, back_inserter( vals[i] ) );
				error |= vals[0].size() != vals[i].size();
			}
			if( error ) {
				return 1;
			}

			ostringstream buf;
			for( size_t j = 0; j < vals[0].size(); ++j ) {
				for( size_t i = 0; i < vals.size(); ++i ) {
					buf << vals[i][j] << separator;
				}
			}
			writeAll( ofd, buf.str() );

			return 0;
		}
		VCASE( Defer, s ) {
			deque<string> args;
			evalArgs( s->args.get(), local, back_inserter( args ) );

			lock_guard<mutex> lock( mutexGlobal );
			local->defs.push_back( move( args ) );
			return 0;
		}
		VCASE( ChDir, s ) {
			deque<string> args;
			evalArgs( s->args.get(), local, back_inserter( args ) );
			if( args.size() != 1 ) {
				throw invalid_argument( "" );
			}

			lock_guard<mutex> lock( mutexGlobal );
			local->cwd = args[0];
			return 0;
		}
		VCASE( None, s ) {
			return s->retv;
		}
		VDEFAULT {
			assert( false );
		}
	} }
	catch( invalid_argument const& ) {
		return -1;
	}
	catch( system_error const& ) {
		return -2;
	}
	catch( ios_base::failure const& ) {
		return -2;
	}

	assert( false );
	return -1;
}

inline future<int> Evaluator::evaluate( ast::Stmt* stmt, shared_ptr<Local> local, int ifd, int ofd ) {
	return async( [&]() -> int {
		return evalStmt( stmt, local, ifd, ofd );
	} );
}

inline void Evaluator::join() {
	while( true ) {
		map<thread::id, thread> fg;
		map<thread::id, thread> bg;
		{
			lock_guard<mutex> lock( mutexGlobal );
			swap( fg, foregrounds );
			swap( bg, backgrounds );
		}

		if( fg.empty() && bg.empty() ) {
			break;
		}

		for( auto& t: fg ) {
			t.second.join();
		}
		for( auto& t: bg ) {
			t.second.join();
		}
	}
}

inline void Evaluator::interrupt() {
	lock_guard<mutex> lock( mutexGlobal );
	for( auto& t: foregrounds ) {
		pthread_kill( t.second.native_handle(), SIGUSR1 );
	}
	for( auto& t: backgrounds ) {
		pthread_kill( t.second.native_handle(), SIGUSR1 );
	}
}
