// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once


// broken, broken, broken
struct Repl {
	Repl() {
		assert( self() == nullptr );
		self() = this;
		local = make_shared<Evaluator::Local>();

		looper.addSignal( SIGTSTP, bind( &Repl::onSignalStp, this ) );
		looper.addSignal( SIGINT , bind( &Repl::onSignalInt, this ) );
		looper.addReader( 0, bind( &Repl::onStdin, this ) );
		rl_callback_handler_install( "| ", &Repl::onLineEntered );

		finished = false;
		while( !finished ) {
			looper.wait();
		}

		rl_callback_handler_remove();
	}

	void onSignalStp() {
	}

	void onSignalInt() {
		cout << "\r\x1b[K" << flush;
		rl_initialize();
		rl_redisplay();
	}

	void onStdin() {
		rl_callback_read_char();
	}

	static void onLineEntered( char* line ) {
		// Ctrl-D
		if( line == nullptr ) {
			cout << "\nwaiting background jobs..." << flush;
			self()->eval.join();
			self()->finished = true;
			return;
		}
		auto deleter = scopeExit( bind( free, line ) );

		size_t len = strlen( line );
		if( len > 0 ) {
			add_history( line );
		}

		try {
			istringstream istr( line );
			unique_ptr<ast::Stmt> ast = parse( istr );

			int retv = self()->eval.evalStmt( ast.get(), self()->local, 0, 1 );
			if( retv != 0 ) {
				cerr << "The command returned " << retv << "." << endl;
			}
		}
		catch( SyntaxError const& ) {
			cerr << "Syntax error." << endl;
		}
		catch( ThreadSupport::Interrupt const& ) {
			cerr << "\nInterrupted." << endl;
		}
		catch( system_error const& ) {
			cerr << "I/O error." << endl;
		}
		catch( ios_base::failure const& ) {
			cerr << "I/O error." << endl;
		}
	}

	static Repl*& self() {
		static __thread Repl* s = nullptr;
		return s;
	}

	EventLooper looper;
	Evaluator eval;
	shared_ptr<Evaluator::Local> local;
	bool finished;
};
