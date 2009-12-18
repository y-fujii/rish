#pragma once

#include "ast.hpp"


#define MATCH( c ) break; case (c):
#define OTHERWISE break; default:

void evalStatement( Statement* s ) {
	switch( s->type ) {
		MATCH( ast::tSequence ) {
			Sequence* s = static_cast<ast::Sequence*>( s );
			eval( s->lhs );
			eval( s->rhs );
		}
		MATCH( ast::tCommand ) {
			Command* s = static_cast<ast::Command*>( s );

			char** args = new char*[s->args->size() + 1];
			for( int i = 0; i < s->args->size(); ++i ) {
				args[i] = evalValue( s->args[i] )->c_ptr();
			}
			args[s->args->size() + 1] = NULL;

			pid_t pid = fork();
			if( pid < 0 ) {
				throw std::exception();
			}
			else if( pid == 0 ) {
				execv( s->cmd->c_ptr(), args );
				throw std::exception();
			}
			if( waitpid( pid, NULL, 0 ) < 0 ) {
				throw std::exception();
			}
		}
		OTHERWISE {
			assert( false );
		}
	}
}

SharedArray evalValue( Value* v ) {
	switch( s->type ) {
		MATCH( ast::tWord ) {
			SharedArray arr;
			arr.push_back( s->word );
			return arr;
		}
		MATCH( ast::tList ) {
			return flatten( s->vals );
		}
		OTHERWISE {
			assert( false );
		}
	}
}
