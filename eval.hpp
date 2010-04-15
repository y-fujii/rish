#pragma once

#include <string>
#include <ext/rope>
#include <deque>
#include <unistd.h>
#include <sys/wait.h>
#include "ast.hpp"
#include "compat.hpp"


#define MATCH( c ) break; case (c):
#define OTHERWISE break; default:

void evalValue( std::deque<std::string>& dst, ast::Expr* e ) {
	using namespace std;
	using namespace ast;

	switch( e->tag ) {
		MATCH( tWord ) {
			Word* w = static_cast<Word*>( e );
			dst.push_back( *w->word );
		}
		MATCH( tList ) {
			List* l = static_cast<List*>( e );
			for( auto i = l->vals->begin(); i != l->vals->end(); ++i ) {
				evalValue( dst, *i );
			}
		}
		OTHERWISE {
			assert( false );
		}
	}
}

void evalStatement( ast::Statement* s ) {
	using namespace std;
	using namespace ast;

	switch( s->tag ) {
		MATCH( tSequence ) {
			Sequence* t = static_cast<Sequence*>( s );
			evalStatement( t->lhs );
			evalStatement( t->rhs );
		}
		MATCH( tCommand ) {
			Command* c = static_cast<Command*>( s );

			deque<string> args;
			evalValue( args, c->args );

			char const** args_raw = new char const*[args.size() + 1];
			for( size_t i = 0; i < args.size(); ++i ) {
				args_raw[i] = args[i].c_str();
			}
			args_raw[args.size()] = NULL;


			pid_t pid = fork();
			if( pid < 0 ) {
				throw exception();
			}
			else if( pid == 0 ) {
				execv( args_raw[0], const_cast<char* const*>( &args_raw[1] ) );
				throw exception();
			}
			if( waitpid( pid, NULL, 0 ) < 0 ) {
				throw exception();
			}
		}
		OTHERWISE {
			assert( false );
		}
	}
}
