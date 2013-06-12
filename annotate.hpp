// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <map>
#include "ast.hpp"
#include "misc.hpp"
#include "parser.hpp"

using namespace std;


struct Annotator {
	struct Local {
		Local(): outer( nullptr ) {}

		void value( ast::Var* var ) {
			if( !lookup( var ) ) {
				throw SyntaxError( var->line );
			}
		}

		void assign( ast::Var* var ) {
			if( !lookup( var ) ) {
				size_t idx = vars.size();
				vars[var->name] = idx;
				assert( vars.size() == idx + 1 );
				var->depth = 0;
				var->index = idx;
			}
		}

		bool lookup( ast::Var* var ) {
			int depth = 0;
			Local* lit = this;
			while( lit != nullptr ) {
				auto vit = lit->vars.find( var->name );
				if( vit != lit->vars.end() ) {
					var->depth = depth;
					var->index = vit->second;
					return true;
				}
				++depth;
				lit = lit->outer;
			}
			return false;
		}

		Local* outer;
		map<string, int> vars;
	};

	void annotate( ast::Expr*, Local& );
	void annotate( ast::LeftExpr*, Local& );
	void annotate( ast::Stmt*, Local& );
};

void Annotator::annotate( ast::Expr* expr, Annotator::Local& local ) {
	using namespace ast;

	VSWITCH( expr ) {
		VCASE( Word, e ) {
		}
		VCASE( Pair, e ) {
			annotate( e->lhs.get(), local );
			annotate( e->rhs.get(), local );
		}
		VCASE( Concat, e ) {
			annotate( e->lhs.get(), local );
			annotate( e->rhs.get(), local );
		}
		VCASE( Var, e ) {
			local.value( e );
		}
		VCASE( Subst, e ) {
			annotate( e->body.get(), local );
		}
		VCASE( Null, _ ) {
		}
		VDEFAULT {
			assert( false );
		}
	}
}

void Annotator::annotate( ast::LeftExpr* expr, Local& local ) {
	using namespace ast;

	VSWITCH( expr ) {
		VCASE( VarFix, e ) {
			for( auto& v: e->var ) {
				if( auto var = match<Var>( v.get() ) ) {
					local.assign( var );
				}
			}
		}
		VCASE( VarVar, e ) {
			for( auto& v: e->varL ) {
				if( auto var = match<Var>( v.get() ) ) {
					local.assign( var );
				}
			}
			local.assign( e->varM.get() );
			for( auto& v: e->varR ) {
				if( auto var = match<Var>( v.get() ) ) {
					local.assign( var );
				}
			}
		}
		VDEFAULT {
			assert( false );
		}
	}
}

void Annotator::annotate( ast::Stmt* stmt, Local& local ) {
	using namespace ast;

tailRec:
	VSWITCH( stmt ) {
		VCASE( Sequence, s ) {
			annotate( s->lhs.get(), local );
			annotate( s->rhs.get(), local );
		}
		VCASE( Parallel, s ) {
			annotate( s->lhs.get(), local );
			annotate( s->rhs.get(), local );
		}
		VCASE( Bg, s ) {
			annotate( s->body.get(), local );
		}
		VCASE( RedirFr, s ) {
			annotate( s->file.get(), local );
			annotate( s->body.get(), local );
		}
		VCASE( RedirTo, s ) {
			annotate( s->file.get(), local );
			annotate( s->body.get(), local );
		}
		VCASE( Command, s ) {
			annotate( s->args.get(), local );
		}
		VCASE( Return, s ) {
			annotate( s->retv.get(), local );
		}
		VCASE( Fun, s ) {
			annotate( s->name.get(), local );
			Local child;
			annotate( s->args.get(), child );
			child.outer = &local;
			annotate( s->body.get(), child );
			s->nVar = child.vars.size();
		}
		VCASE( FunDel, s ) {
			annotate( s->name.get(), local );
		}
		VCASE( If, s ) {
			annotate( s->cond.get(), local );
			annotate( s->then.get(), local );
			annotate( s->elze.get(), local );
		}
		VCASE( While, s ) {
			annotate( s->cond.get(), local );
			annotate( s->body.get(), local );
			annotate( s->elze.get(), local );
		}
		VCASE( Break, s ) {
			annotate( s->retv.get(), local );
		}
		VCASE( Let, s ) {
			annotate( s->lhs.get(), local );
			annotate( s->rhs.get(), local );
		}
		VCASE( Fetch, s ) {
			annotate( s->lhs.get(), local );
		}
		VCASE( Yield, s ) {
			annotate( s->rhs.get(), local );
		}
		VCASE( Pipe, s ) {
			annotate( s->lhs.get(), local );
			annotate( s->rhs.get(), local );
		}
		VCASE( Zip, s ) {
			for( auto& e: s->exprs ) {
				annotate( e.get(), local );
			}
		}
		VCASE( Defer, s ) {
			annotate( s->args.get(), local );
		}
		VCASE( None, s ) {
		}
		VDEFAULT {
			assert( false );
		}
	}
}
