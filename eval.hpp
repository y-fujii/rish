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
	Evaluator(): stdin( 0 ), stdout( 1 ), stderr( 2 ) {} // XXX

	using ArgIter = move_iterator<deque<string>::iterator>;
	using Builtin = function<int ( ArgIter, ArgIter, Evaluator&, int, int )>;

	struct Local {
		deque<string>& value( ast::Var* );
		template<class Iter> bool assign( ast::VarFix*, Iter, Iter );
		template<class Iter> bool assign( ast::VarVar*, Iter, Iter );
		template<class Iter> bool assign( ast::LeftExpr*, Iter, Iter );

		shared_ptr<Local> outer;
		map<string, deque<string>> vars;
		deque<deque<string>> defs;
	};

	struct Closure {
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

	struct ArgError {
	};

	template<class Iter> int callCommand( Iter, Iter, int, int );
	template<class Iter> Iter evalExpr( ast::Expr*, shared_ptr<Local>, Iter );
	template<class Iter> Iter evalArgs( ast::Expr*, shared_ptr<Local>, Iter );
	int evalStmt( ast::Stmt*, shared_ptr<Local>, int, int );
	future<int> evaluate( ast::Stmt*, shared_ptr<Local> );
	void join();
	void interrupt();

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
		VCASE( Null, _ ) {
		}
		VDEFAULT {
			assert( false );
		}
	}
	return dst;
}

template<class Iter>
int Evaluator::callCommand( Iter argsB, Iter argsE, int ifd, int ofd ) {
	assert( argsE - argsB >= 1 );

	mutexGlobal.lock();
	auto fit = closures.find( argsB[0] );
	if( fit != closures.end() ) {
		Closure cl = fit->second;
		mutexGlobal.unlock();

		auto local = make_shared<Local>();
		if( !local->assign( cl.args.get(), argsB + 1, argsE ) ) {
			throw ArgError(); // or allow overloaded functions?
		}
		local->outer = move( cl.env );

		int retv;
		try {
			retv = evalStmt( cl.body.get(), local, ifd, ofd );
		}
		catch( ReturnException const& e ) {
			retv = e.retv;
		}

		for( auto it = local->defs.rbegin(); it != local->defs.rend(); ++it ) {
			callCommand(
				make_move_iterator( it->begin() ),
				make_move_iterator( it->end() ),
				ifd, ofd
			);
		}
		// loca.defs is not required anymore but local itself may be referenced
		// by other closures.
		local->defs = {};

		return retv;
	}
	else {
		mutexGlobal.unlock();
	}

	auto bit = builtins.find( argsB[0] );
	if( bit != builtins.end() ) {
		try {
			return bit->second( argsB + 1, argsE, *this, ifd, ofd );
		}
		catch( std::exception const& ) {
			return 1;
		}
	}

	pid_t pid = forkExec( argsB, argsE, ifd, ofd );
	int status;
	checkSysCall( waitpid( pid, &status, 0 ) );
	return WEXITSTATUS( status );
}

template<class DstIter>
DstIter Evaluator::evalArgs( ast::Expr* expr, shared_ptr<Local> local, DstIter dstIt ) {
	struct Inserter: std::iterator<output_iterator_tag, Inserter> {
		DstIter dstIt;

		explicit Inserter( DstIter it ): dstIt( it ) {
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
			shared_ptr<Stmt> body = s->body;
			thread thr( [=]() -> void {
				// keep the reference to AST
				this->evalStmt( body.get(), local, this->stdin, this->stdout );
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
			evalArgs( s->file.get(), local, back_inserter( args ) );
			if( args.size() != 1 ) {
				throw ArgError();
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
				throw ArgError();
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
				ifd, ofd
			);
		}
		VCASE( Return, s ) {
			deque<string> args;
			evalArgs( s->retv.get(), local, back_inserter( args ) );
			if( args.size() != 1 ) {
				throw ArgError();
			}

			throw ReturnException( readValue<int>( args[0] ) ) ;
		}
		VCASE( Fun, s ) {
			deque<string> args;
			evalArgs( s->name.get(), local, back_inserter( args ) );
			if( args.size() != 1 ) {
				throw ArgError();
			}

			lock_guard<mutex> lock( mutexGlobal );
			closures[args[0]] = { s->args, s->body, local };
			return 0;
		}
		VCASE( FunDel, s ) {
			deque<string> args;
			evalArgs( s->name.get(), local, back_inserter( args ) );
			if( args.size() != 1 ) {
				throw ArgError();
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
			if( args.size() != 1 ) {
				throw ArgError();
			}
			int retv = readValue<int>( args[0] );
			throw BreakException( retv );
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
					UnixIStream ifs( ifd, 1 );
					deque<string> rhs( lhs->var.size() );
					for( auto& v: rhs ) {
						if( !getline( ifs, v ) ) {
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
					UnixIStream ifs( ifd );
					string buf;
					while( getline( ifs, buf ) ) {
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
				buf << v << '\n';
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
		VCASE( Defer, s ) {
			deque<string> args;
			evalArgs( s->args.get(), local, back_inserter( args ) );

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
	catch( ArgError const& ) {
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

inline future<int> Evaluator::evaluate( ast::Stmt* stmt, shared_ptr<Local> local ) {
	return async( [&]() -> int {
		return evalStmt( stmt, local, stdin, stdout );
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
