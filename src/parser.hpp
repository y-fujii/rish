// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once


struct SyntaxError: exception {
	SyntaxError( size_t l ): line( l ) {}

	size_t line;
};

void lexerInit( istream* );
size_t lexerLineNo();
unique_ptr<ast::Stmt> parse( istream& );
