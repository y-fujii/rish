// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license

#include "pch.hpp"
#include "misc.hpp"


void cmdChars( char**, int ) {
	char c;
	while( cin.read( &c, 1 ) ) {
		cout << c << '\n';
	}
}

void cmdJoin( char**, int ) {
	string buf;
	while( getline( cin, buf ) ) {
		cout << buf;
	}
	cout << '\n';
}

void cmdLen( char**, int ) {
	string buf;
	while( getline( cin, buf ) ) {
		cout << buf.size() << '\n';
	}
}

void cmdIndex( char** arg, int argc ) {
	vector<int64_t> idx( argc );
	for( int i = 0; i < argc; ++i ) {
		idx[i] = strtoll( arg[i], nullptr, 10 );
	}

	string buf;
	while( getline( cin, buf ) ) {
		for( int64_t i: idx ) {
			if( buf.size() == 0 ) {
				throw exception();
			}
			int64_t j = imod( i, buf.size() );
			cout << buf[j] << '\n';
		}
	}
}

void cmdSlice( char** arg, int ) {
	int64_t bgn = strtoll( arg[0], nullptr, 10 );
	int64_t end = strtoll( arg[1], nullptr, 10 );

	string buf;
	while( getline( cin, buf ) ) {
		int64_t b = imod( bgn, buf.size() );
		int64_t e = imod( end, buf.size() );
		if( b > e ) {
			throw exception();
		}
		cout << buf.substr( b, e - b ) << '\n';
	}
}

void cmdCmp( char** arg, int ) {
	string src( arg[0] );

	string buf;
	while( getline( cin, buf ) ) {
		int t = (
			buf < src ? -1 :
			buf > src ? +1 :
						 0
		);
		cout << t << '\n';
	}
}

void cmdMatch( char** arg, int ) {
	regex re( arg[0] );
	string buf;
	while( getline( cin, buf ) ) {
		if( regex_match( buf, re ) ) {
			cout << buf << '\n';
		}
	}
}

void cmdSubst( char** arg, int ) {
	regex re( arg[0] );
	string buf;
	while( getline( cin, buf ) ) {
		cout << regex_replace( buf, re, arg[1] ) << '\n';
	}
}

struct Command {
	string name;
	int arity;
	function<void( char**, int )> func;
};

array<Command, 8> commands = {{
	{ "chars",  0, &cmdChars },
	{ "join" ,  0, &cmdJoin  },
	{ "len"  ,  0, &cmdLen   },
	{ "index", -1, &cmdIndex },
	{ "slice",  2, &cmdSlice },
	{ "cmp"  ,  1, &cmdCmp   },
	{ "match",  1, &cmdMatch },
	{ "subst",  2, &cmdSubst },
}};

int main( int argc, char** argv ) {
	try {
		int opt;
		while( opt = getopt( argc, argv, "" ), opt != -1 ) {
			throw exception();
		}
		if( optind >= argc ) {
			throw exception();
		}

		string cmd = argv[optind++];
		int n = argc - optind;

		for( auto&& e: commands ) {
			if( cmd == e.name ) {
				if( e.arity >= 0 && e.arity != n ) {
					throw exception();
				}
				e.func( argv + optind, n );
				return 0;
			}
		}
	}
	catch( exception ) {
	}

	cerr << "error.\n";
	return -1;
}

