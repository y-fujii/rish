// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <map>
#include "ast.hpp"
#include "misc.hpp"
#include "parser.hpp"

using namespace std;


struct Annotator: ast::Visitor<Annotator> {
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

	using ast::Visitor<Annotator>::operator();

	void operator()( ast::Var* e, Local& local ) {
		local.value( e );
	}

	void operator()( ast::LeftFix* e, Local& local ) {
		for( auto& v: e->var ) {
			if( auto var = match<ast::Var>( v.get() ) ) {
				local.assign( var );
			}
		}
	}

	void operator()( ast::LeftVar* e, Local& local ) {
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

	void operator()( ast::Fun* s, Local& local ) {
		(*this)( s->name.get(), local );
		Local child;
		(*this)( s->args.get(), child );
		child.outer = &local;
		(*this)( s->body.get(), child );
		s->nVar = child.vars.size();
	}
};

inline void annotate( ast::Stmt* s, Annotator::Local& local ) {
	Annotator()( s, local );
}
