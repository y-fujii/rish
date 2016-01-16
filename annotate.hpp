// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once


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

	void operator()( ast::Expr* expr, Local& local ) {
		VSWITCH( expr ) {
			VCASE( ast::Var, e ) {
				local.value( e );
			}
			VDEFAULT {
				ast::walk( *this, expr, local );
			}
		}
	}

	void operator()( ast::LeftExpr* expr, Local& local ) {
		VSWITCH( expr ) {
			VCASE( ast::LeftFix, e ) {
				for( auto& v: e->var ) {
					if( auto var = match<ast::Var>( v.get() ) ) {
						local.assign( var );
					}
				}
			}
			VCASE( ast::LeftVar, e ) {
				for( auto& v: e->varL ) {
					if( auto var = match<ast::Var>( v.get() ) ) {
						local.assign( var );
					}
				}
				local.assign( e->varM.get() );
				for( auto& v: e->varR ) {
					if( auto var = match<ast::Var>( v.get() ) ) {
						local.assign( var );
					}
				}
			}
			VDEFAULT {
				ast::walk( *this, expr, local );
			}
		}
	}

	void operator()( ast::Stmt* stmt, Local& local ) {
		VSWITCH( stmt ) {
			VCASE( ast::Fun, s ) {
				(*this)( s->name.get(), local );
				Local child;
				(*this)( s->args.get(), child );
				child.outer = &local;
				(*this)( s->body.get(), child );
				s->nVar = child.vars.size();
			}
			VDEFAULT {
				ast::walk( *this, stmt, local );
			}
		}
	}
};

inline void annotate( ast::Stmt* s, Annotator::Local& local ) {
	Annotator()( s, local );
}
