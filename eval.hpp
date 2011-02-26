// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
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
#include <errno.h>
#include <sys/wait.h>
#include "ast.hpp"
#include "command.hpp"
#include "glob.hpp"
#include "misc.hpp"

using namespace std;


struct Global {
	map<string, deque<string> > vars; // this will be removed
	map<string, ast::Fun*> funs;
	//map<string, pair<ast::Fun*, Local*> > funs;
};

struct Local {
	Local( Local* o = nullptr ): outer( o ) {}

	Local* outer;
	map<string, deque<string> > vars;
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

int evalStmtClose( ast::Stmt*, Global*, Local*, int, int, atomic<bool>&, bool, bool );


inline void checkSysCall( int retv ) {
	if( retv < 0 ) {
		if( errno == EINTR ) {
			throw StopException();
		}
		else {
			throw IOError();
		}
	}
}

inline deque<string>& findVariable( string const& name, Global* global, Local* local ) {
	Local* it = local;
	while( it != nullptr ) {
		map<string, deque<string> >::iterator v = it->vars.find( name );
		if( v != it->vars.end() ) {
			return v->second;
		}
		it = local->outer;
	}

	map<string, deque<string> >::iterator v = global->vars.find( name );
	if( v != global->vars.end() ) {
		return v->second;
	}

	if( local != nullptr ) {
		return local->vars[name];
	}
	else {
		return global->vars[name];
	}
}

// XXX
template<class Container>
bool assign( ast::VarFix* lhs, Container& rhs, Global* global, Local* local ) {
	using namespace ast;

	for( size_t i = 0; i < rhs.size(); ++i ) {
		VARIANT_SWITCH( Expr, lhs->var[i] ) {
			VARIANT_CASE( Word, word ) {
				if( word->word != MetaString( rhs[i] ) ) {
					return false;
				}
			}
		}
	}

	for( size_t i = 0; i < rhs.size(); ++i ) {
		VARIANT_SWITCH( Expr, lhs->var[i] ) {
			VARIANT_CASE( Var, tvar ) {
				deque<string>& vvar = findVariable( tvar->name, global, local );
				vvar.clear();
				vvar.push_back( rhs[i] );
			}
		}
	}

	return true;
}

// XXX
template<class Container>
bool assign( ast::VarVar* lhs, Container& rhs, Global* global, Local* local ) {
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
		}
	}
	for( size_t i = 0; i < lhs->varR.size(); ++i ) {
		VARIANT_SWITCH( Expr, lhs->varR[i] ) {
			VARIANT_CASE( Word, word ) {
				if( word->word != MetaString( rhs[i + rBgn] ) ) {
					return false;
				}
			}
		}
	}

	for( size_t i = 0; i < lhs->varL.size(); ++i ) {
		VARIANT_SWITCH( Expr, lhs->varL[i] ) {
			VARIANT_CASE( Var, tvar ) {
				deque<string>& vvar = findVariable( tvar->name, global, local );
				vvar.clear();
				vvar.push_back( rhs[i + lBgn] );
			}
		}
	}
	deque<string>& vvar = findVariable( lhs->varM->name, global, local );
	vvar.clear();
	copy( &rhs[mBgn], &rhs[rBgn], back_inserter( vvar ) );
	for( size_t i = 0; i < lhs->varR.size(); ++i ) {
		VARIANT_SWITCH( Expr, lhs->varR[i] ) {
			VARIANT_CASE( Var, tvar ) {
				deque<string>& vvar = findVariable( tvar->name, global, local );
				vvar.clear();
				vvar.push_back( rhs[i + rBgn] );
			}
		}
	}

	return true;
}

template<class Container>
bool assign( ast::LeftExpr* lhsb, Container& rhs, Global* global, Local* local ) {
	using namespace ast;

	VARIANT_SWITCH( LeftExpr, lhsb ) {
		VARIANT_CASE( VarFix, lhs ) {
			if( rhs.size() != lhs->var.size() ) {
				return false;
			}
			return assign( lhs, rhs, global, local );
		}
		VARIANT_CASE( VarVar, lhs ) {
			if( rhs.size() < lhs->varL.size() + lhs->varR.size() ) {
				return false;
			}
			return assign( lhs, rhs, global, local );
		}
		OTHERWISE {
			assert( false );
		}
	}
}

template<class DstIter>
DstIter evalExpr( ast::Expr* eb, Global* global, Local* local, DstIter dst, atomic<bool>& stop ) {
	using namespace ast;

	if( stop.load() ) {
		throw StopException();
	}

	VARIANT_SWITCH( Expr, eb ) {
		VARIANT_CASE( Word, e ) {
			*dst++ = e->word;
		}
		VARIANT_CASE( Pair, e ) {
			evalExpr( e->lhs, global, local, dst, stop );
			evalExpr( e->rhs, global, local, dst, stop );
		}
		VARIANT_CASE( Null, e ) {
		}
		VARIANT_CASE( Concat, e ) {
			deque<MetaString> lhs;
			deque<MetaString> rhs;
			evalExpr( e->lhs, global, local, back_inserter( lhs ), stop );
			evalExpr( e->rhs, global, local, back_inserter( rhs ), stop );
			for( deque<MetaString>::const_iterator i = lhs.begin(); i != lhs.end(); ++i ) {
				for( deque<MetaString>::const_iterator j = rhs.begin(); j != rhs.end(); ++j ) {
					*dst++ = *i + *j;
				}
			}
		}
		VARIANT_CASE( Var, e ) {
			deque<string>& val = findVariable( e->name, global, local );
			dst = copy( val.begin(), val.end(), dst );
		}
		VARIANT_CASE( Subst, e ) {
			int fds[2];
			checkSysCall( pipe( fds ) );
			Thread thread( bind( evalStmtClose, e->body, global, local, 0, fds[1], stop, false, true ) );
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
		OTHERWISE {
			assert( false );
		}
	}
	return dst;
}

template<class DstIter>
DstIter evalArgs( ast::Expr* expr, Global* global, Local* local, DstIter dstIt, atomic<bool>& stop ) {
	deque<MetaString> tmp;
	evalExpr( expr, global, local, back_inserter( tmp ), stop );
	return accumulate( tmp.begin(), tmp.end(), dstIt, bind( expandGlob<DstIter>, _2, _1 ) );
}

int evalStmt( ast::Stmt* sb, Global* global, Local* local, int ifd, int ofd, atomic<bool>& stop ) {
	using namespace ast;

	if( stop.load() ) {
		throw StopException();
	}

	try { VARIANT_SWITCH( Stmt, sb ) {
		VARIANT_CASE( Sequence, s ) {
			evalStmt( s->lhs, global, local, ifd, ofd, stop );
			return evalStmt( s->rhs, global, local, ifd, ofd, stop );
		}
		VARIANT_CASE( RedirFr, s ) {
			deque<string> args;
			evalArgs( s->file, global, local, back_inserter( args ), stop );
			int fd = open( args.back().c_str(), O_RDONLY );
			checkSysCall( fd );
			ScopeExit closer( bind( close, fd ) );
			return evalStmt( s->body, global, local, fd, ofd, stop );
		}
		VARIANT_CASE( RedirTo, s ) {
			deque<string> args;
			evalArgs( s->file, global, local, back_inserter( args ), stop );
			int fd = open( args.back().c_str(), O_WRONLY | O_CREAT, 0644 );
			checkSysCall( fd );
			ScopeExit closer( bind( close, fd ) );
			return evalStmt( s->body, global, local, ifd, fd, stop );
		}
		VARIANT_CASE( Command, s ) {
			deque<string> args;
			evalArgs( s->args, global, local, back_inserter( args ), stop );
			if( args.size() == 0 ) {
				return 0;
			}

			map<string, Fun*>::const_iterator it = global->funs.find( args[0] );
			if( it != global->funs.end() ) {
				try {
					Local local;
					args.pop_front();
					if( assign( it->second->args, args, global, &local ) ) {
						return evalStmt( it->second->body, global, &local, ifd, ofd, stop );
					}
					else {
						assert( false ); // to be implemented
					}
				}
				catch( ReturnException const& e ) {
					return e.retv;
				}
			}
			else {
				return runCommand( args, ifd, ofd );
			}
		}
		VARIANT_CASE( Return, s ) {
			deque<string> args;
			evalArgs( s->retv, global, local, back_inserter( args ), stop );
			if( args.size() == 0 ) {
				return 1;
			}
			int retv;
			if( (istringstream( args.back() ) >> retv).fail() ) {
				return 1;
			}
			throw ReturnException( retv ) ;
		}
		VARIANT_CASE( Fun, s ) {
			global->funs[s->name] = s;
			return 0;
		}
		VARIANT_CASE( If, s ) {
			if( evalStmt( s->cond, global, local, ifd, ofd, stop ) == 0 ) {
				return evalStmt( s->then, global, local, ifd, ofd, stop );
			}
			else {
				return evalStmt( s->elze, global, local, ifd, ofd, stop );
			}
		}
		VARIANT_CASE( While, s ) {
			try {
				while( evalStmt( s->cond, global, local, ifd, ofd, stop ) == 0 ) {
					evalStmt( s->body, global, local, ifd, ofd, stop );
				}
			}
			catch( BreakException const& e ) {
				return e.retv;
			}
			return evalStmt( s->elze, global, local, ifd, ofd, stop );
		}
		VARIANT_CASE( Break, s ) {
			deque<string> args;
			evalArgs( s->retv, global, local, back_inserter( args ), stop );
			int retv;
			if( (istringstream( args.back() ) >> retv).fail() ) {
				return 1;
			}
			throw BreakException( retv );
		}
		VARIANT_CASE( Let, s ) {
			deque<string> vals;
			evalArgs( s->rhs, global, local, back_inserter( vals ), stop );
			return assign( s->lhs, vals, global, local ) ? 0 : 1;
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

					return assign( lhs, rhs, global, local ) ? 0 : 1;
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

					return assign( lhs, rhs, global, local ) ? 0 : 1;
				}
			}
		}
		VARIANT_CASE( Pipe, s ) {
			int fds[2];
			checkSysCall( pipe( fds ) );
			Thread thread( bind( evalStmtClose, s->lhs, global, local, ifd, fds[1], stop, false, true ) );
			int retv;
			{
				ScopeExit closer( bind( close, fds[0] ) );
				retv = evalStmt( s->rhs, global, local, fds[0], ofd, stop );
			}
			thread.join();

			return retv;
		}
		/*
		VARIANT_CASE( For, s ) {
			return 0;
		}
		*/
		VARIANT_CASE( None, s ) {
			return 0;
		}
		OTHERWISE {
			assert( false );
		}
	} }
	catch( IOError const& ) {
		return 1;
	}

	assert( false );
}

int evalStmtClose( ast::Stmt* stmt, Global* global, Local* local, int ifd, int ofd, atomic<bool>& stop, bool ic = false, bool oc = false ) {
	int retv;
	try {
		retv = evalStmt( stmt, global, local, ifd, ofd, stop );
		if( ic ) {
			close( ifd );
		}
		if( oc ) {
			close( ofd );
		}
	}
	catch( ... ) {
		if( ic ) {
			close( ifd );
		}
		if( oc ) {
			close( ofd );
		}
		throw;
	}

	return retv;
}
