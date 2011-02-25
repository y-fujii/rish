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
#include <sys/wait.h>
#include "ast.hpp"
#include "command.hpp"
#include "glob.hpp"
#include "misc.hpp"

using namespace std;


struct Global {
	map<string, deque<string> > vars;
	map<MetaString, ast::Fun*> funs;
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

void evalStmtClose( ast::Stmt*, Global*, int, int, atomic<bool>&, bool, bool );

// XXX
template<class Container>
bool assign( ast::VarFix const* lhs, Container const& rhs, Global* global ) {
	using namespace ast;

	for( size_t i = 0; i < rhs.size(); ++i ) {
		if( lhs->var[i]->tag == Expr::tWord ) {
			Word* word = static_cast<Word*>( lhs->var[i] );
			if( word->word != MetaString( rhs[i] ) ) {
				return false;
			}
		}
	}

	for( size_t i = 0; i < rhs.size(); ++i ) {
		if( lhs->var[i]->tag == Expr::tVar ) {
			Var* tvar = static_cast<Var*>( lhs->var[i] );
			deque<string>& vvar = global->vars[tvar->name];
			vvar.clear();
			vvar.push_back( rhs[i] );
		}
	}

	return true;
}

// XXX
template<class Container>
bool assign( ast::VarVar const* lhs, Container const& rhs, Global* global ) {
	using namespace ast;

	size_t const lBgn = 0;
	size_t const mBgn = lhs->varL.size();
	size_t const rBgn = rhs.size() - lhs->varR.size();

	for( size_t i = 0; i < lhs->varL.size(); ++i ) {
		if( lhs->varL[i]->tag == Expr::tWord ) {
			Word* word = static_cast<Word*>( lhs->varL[i] );
			if( word->word != MetaString( rhs[i + lBgn] ) ) {
				return false;
			}
		}
	}
	for( size_t i = 0; i < lhs->varR.size(); ++i ) {
		if( lhs->varR[i]->tag == Expr::tWord ) {
			Word* word = static_cast<Word*>( lhs->varR[i] );
			if( word->word != MetaString( rhs[i + rBgn] ) ) {
				return false;
			}
		}
	}

	for( size_t i = 0; i < lhs->varL.size(); ++i ) {
		if( lhs->varL[i]->tag == Expr::tVar ) {
			Var* tvar = static_cast<Var*>( lhs->varL[i] );
			deque<string>& vvar = global->vars[tvar->name];
			vvar.clear();
			vvar.push_back( rhs[i + lBgn] );
		}
	}
	deque<string>& vvar = global->vars[lhs->varM->name];
	vvar.clear();
	copy( &rhs[mBgn], &rhs[rBgn], back_inserter( vvar ) );
	for( size_t i = 0; i < lhs->varR.size(); ++i ) {
		if( lhs->varR[i]->tag == Expr::tVar ) {
			Var* tvar = static_cast<Var*>( lhs->varR[i] );
			deque<string>& vvar = global->vars[tvar->name];
			vvar.clear();
			vvar.push_back( rhs[i + rBgn] );
		}
	}

	return true;
}

template<class DstIter>
DstIter evalExpr( ast::Expr* eb, Global* global, DstIter dst, atomic<bool>& stop ) {
	using namespace ast;

	if( stop.load() ) {
		throw StopException();
	}

	switch( eb->tag ) {
		MATCH( Expr::tWord ) {
			Word* e = static_cast<Word*>( eb );
			*dst++ = e->word;
		}
		MATCH( Expr::tPair ) {
			Pair* e = static_cast<Pair*>( eb );
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
			if( it != global->vars.end() ) {
				dst = copy( it->second.begin(), it->second.end(), dst );
			}
		}
		MATCH( Expr::tSubst ) {
			Subst* e = static_cast<Subst*>( eb );

			int fds[2];
			checkSysCall( pipe( fds ) );
			Thread thread( bind( evalStmtClose, e->body, global, 0, fds[1], stop, false, true ) );
			{
				UnixIStream ifs( fds[0] );
				string buf;
				while( getline( ifs, buf ) ) {
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
DstIter evalArgs( ast::Expr* expr, Global* global, DstIter dstIt, atomic<bool>& stop ) {
	deque<MetaString> tmp;
	evalExpr( expr, global, back_inserter( tmp ), stop );
	return accumulate( tmp.begin(), tmp.end(), dstIt, bind( expandGlob<DstIter>, _2, _1 ) );
}

int evalStmt( ast::Stmt* sb, Global* global, int ifd, int ofd, atomic<bool>& stop ) {
	using namespace ast;

	if( stop.load() ) {
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
			checkSysCall( fd );
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
			RedirTo* s = static_cast<RedirTo*>( sb );
			deque<string> args;
			evalArgs( s->file, global, back_inserter( args ), stop );
			int fd = open( args.back().c_str(), O_WRONLY | O_CREAT, 0644 );
			checkSysCall( fd );
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
			
			assert( args.size() >= 1 );
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
			throw BreakException( retv );
		}
		MATCH( Stmt::tLet ) {
			Let* s = static_cast<Let*>( sb );
			deque<string> rhs;
			evalArgs( s->rhs, global, back_inserter( rhs ), stop );
			switch( s->lhs->tag ) {
				MATCH( LeftExpr::tVarFix ) {
					VarFix* lhs = static_cast<VarFix*>( s->lhs );
					if( rhs.size() != lhs->var.size() ) {
						return 1;
					}
					return assign( lhs, rhs, global ) ? 0 : 1;
				}
				MATCH( LeftExpr::tVarVar ) {
					VarVar* lhs = static_cast<VarVar*>( s->lhs );
					if( rhs.size() < lhs->varL.size() + lhs->varR.size() ) {
						return 1;
					}
					return assign( lhs, rhs, global ) ? 0 : 1;
				}
				OTHERWISE {
					assert( false );
				}
			}
		}
		MATCH( Stmt::tFetch ) {
			Fetch* s = static_cast<Fetch*>( sb );
			switch( s->lhs->tag ) {
				MATCH( LeftExpr::tVarFix ) {
					VarFix* lhs = static_cast<VarFix*>( s->lhs );

					UnixIStream ifs( ifd, 1 );
					deque<string> rhs( lhs->var.size() );
					for( deque<string>::iterator it = rhs.begin(); it != rhs.end(); ++it ) {
						if( !getline( ifs, *it ) ) {
							return 1;
						}
					}

					return assign( lhs, rhs, global ) ? 0 : 1;
				}
				MATCH( LeftExpr::tVarVar ) {
					VarVar* lhs = static_cast<VarVar*>( s->lhs );

					deque<string> rhs;
					UnixIStream ifs( ifd );
					string buf;
					while( getline( ifs, buf ) ) {
						rhs.push_back( buf );
					}
					if( rhs.size() < lhs->varL.size() + lhs->varR.size() ) {
						return 1;
					}

					return assign( lhs, rhs, global ) ? 0 : 1;
				}
			}
		}
		MATCH( Stmt::tPipe ) {
			Pipe* s = static_cast<Pipe*>( sb );

			int fds[2];
			checkSysCall( pipe( fds ) );
			Thread thread( bind( evalStmtClose, s->lhs, global, ifd, fds[1], stop, false, true ) );
			int retv;
			try {
				retv = evalStmt( s->rhs, global, fds[0], ofd, stop );
			}
			catch( ... ) {
				close( fds[0] );
				throw;
			}
			close( fds[0] );
			thread.join();

			return retv;
		}
		/*
		MATCH( Stmt::tFor ) {
			For* s = static_cast<For*>( sb );

			UnixIStream ifs( ifd, 1 ); // XXX
			while( true ) {
				Expr* it = s->vars;
				while( it->tag == Expr::tPair ) {
					Pair* list = static_cast<Pair*>( it );
					if( list->lhs->tag == Expr::tVar ) {
						Var* var = static_cast<Var*>( list->lhs );
						string lhs;
						if( getline( ifs, lhs ) ) {
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
		*/
		MATCH( Stmt::tNone ) {
			return 0;
		}
		OTHERWISE {
			assert( false );
		}
	}
	assert( false );
}

void evalStmtClose( ast::Stmt* stmt, Global* global, int ifd, int ofd, atomic<bool>& stop, bool ic = false, bool oc = false ) {
	evalStmt( stmt, global, ifd, ofd, stop );
	if( ic ) {
		close( ifd );
	}
	if( oc ) {
		close( ofd );
	}
}
