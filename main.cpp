// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "misc.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "annotate.hpp"
#include "eval.hpp"
//#include "repl.hpp"
//#include "builtins.hpp"

using namespace std;


// toriaezu tekito-
struct TaskManager: Evaluator::Listener {
	using ArgIter = Evaluator::ArgIter;

	TaskManager( char** ab, char** ae ): _argsB( ab ), _argsE( ae ) {}

	virtual int onCommand( ArgIter argsB, ArgIter argsE, int ifd, int ofd, string const& cwd ) override {
		if( argsB[0] == "args" ) {
			ostringstream ofs;
			ofs.exceptions( ios_base::failbit | ios_base::badbit );
			for( char** it = _argsB; it != _argsE; ++it ) {
				ofs << *it << '\n';
			}
			writeAll( ofd, ofs.str() );
			return 0;
		}

		pid_t pid = forkExec( argsB, argsE, ifd, ofd, cwd );
		int status;
		checkSysCall( waitpid( pid, &status, 0 ) );
		return WEXITSTATUS( status );
	}

	virtual void onBgTask( thread&& thr ) override {
		lock_guard<mutex> lock( _mutex );
		_threads.push_back( move( thr ) );
	}

	void join() {
		while( true ) {
			vector<thread> tmp;
			{
				lock_guard<mutex> lock( _mutex );
				swap( tmp, _threads );
			}
			if( tmp.empty() ) {
				break;
			}
			for( auto& t: tmp ) {
				t.join();
			}
		}
	}

	private:
		char** _argsB;
		char** _argsE;
		mutex _mutex;
		vector<thread> _threads;
};

int main( int argc, char** argv ) {
	int opt;
	while( opt = getopt( argc, argv, "" ), opt != -1 ) {
	}

	struct sigaction sa;
	memset( &sa, 0, sizeof( sa ) );
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_RESTART;
	sigaction( SIGPIPE, &sa, nullptr );

	array<char, MAXPATHLEN> buf;
	getcwd( buf.data(), buf.size() );
	string cwd( buf.data() );

	if( optind < argc ) {
		try {
			ifstream ifs( argv[optind] );
			unique_ptr<ast::Stmt> ast = parse( ifs );
			annotate( ast.get() );

			TaskManager taskMan( &argv[optind + 1], &argv[argc] );
			Evaluator eval( &taskMan );
			auto elocal = make_shared<Evaluator::Local>();
			elocal->cwd = cwd;
			eval.evalStmt( ast.get(), elocal, 0, 1 );

			taskMan.join();
		}
		catch( SyntaxError const& err ) {
			cerr << "Syntax error on #" << err.line + 1 << "." << endl;
			return 1;
		}
	}
	/*
	else if( !isatty( 0 ) ) {
		try {
			unique_ptr<ast::Stmt> ast = parse( cin );

			Evaluator eval;
			auto local = make_shared<Evaluator::Local>();
			eval.evalStmt( ast.get(), local, 0, 1 );
			eval.join();
		}
		catch( SyntaxError const& err ) {
			cerr << "Syntax error on #" << err.line + 1 << "." << endl;
			return 1;
		}
	}
	else {
		Repl();
	}
	*/

	return 0;
}
