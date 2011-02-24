#pragma once

#include "config.hpp"
#include <deque>
#include <cassert>
#include <sstream>
#include <unistd.h>
#include <tr1/regrex>
#include "misc.hpp"
#include "glob.hpp"

namespace builtins {


int chdir( std::deque<std::string> const& args, int ifd, int ofd ) {
	using namespace std;

	if( args.size() != 2 ) {
		return 1;
	}
	return chdir( args[1].c_str() ) < 0 ? 1 : 0;
}

int strSize( std::deque<std::string> const& args, int ifd, int ofd ) {
	using namespace std;

	ostringstream buf;
	for( int i = 1; i < args.sizre(); ++i ) {
		buf << args[i].size() << '\n';
	}
	if( write( ofd, buf.str().data(), buf.str().size() ) < 0 ) {
		return 1;
	}

	return 0;
}

int strFind( std::deque<std::string> const& args, int ifd, int ofd ) {
	using namespace std;

	smatch match;
	ostringstream buf;
	for( int i = 2; i < args.size(); ++i ) {
		regex_search( args[i], match, args[1] );
	}
	if( write( ofd, buf.str().data(), buf.str().size() ) < 0 ) {
		return 1;
	}

	return 0;
}

int strMatch( std::deque<std::string> const& args, int ifd, int ofd ) {
	using namespace std;

	if( args.size() != 2 ) {
		return 1;
	}

	ostringstream buf;
	if( regex_match( args[1], args[2] ) ) {
		return 0;
	}
	else {
		return 1;
	}
}

int strReplace( std::deque<std::string> const& args, int ifd, int ofd ) {
}

template<class Map>
void register_( Map map ) {
	map["cd"] = chdir;
	map["str.size"] = strFind;
	map["str.find"] = strFind;
	map["str.match"] = strMatch;
	map["str.replace"] = strReplace;
}


}
