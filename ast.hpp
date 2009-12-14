#pragma once

#include <string>

namespace ast {

struct Ast {
	virtual ~Ast() {
	}
};

struct Word: Ast {
	Word( std::string const& w ):
		word( w ) {
	}

	std::string word;
};

}
