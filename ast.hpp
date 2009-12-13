#pragma once

#include <string>


struct Expr {
	virtual ~Expr() {
	}
};

struct Word: Expr {
	Word( std::string const& w ):
		word( w ) {
	}

	std::string word;
};
