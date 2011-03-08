// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <deque>
#include <cassert>
#include <sstream>
#include <unistd.h>
#include "misc.hpp"
#include "glob.hpp"

using namespace std;

int checkSysCall( int );


struct UnixStreamBuf: std::streambuf {
	UnixStreamBuf( int fd, size_t bs ):
		_fd( fd ), _buf( bs ) {
	}

	virtual int underflow() {
		ssize_t n = read( _fd, &_buf[0], _buf.size() );
		checkSysCall( n );
		if( n == 0 ) {
			setg( nullptr, nullptr, nullptr );
			return traits_type::eof();
		}
		else {
			setg( &_buf[0], &_buf[0], &_buf[n] );
			return _buf[0];
		}
	}

	private:
		int const _fd;
		std::vector<char> _buf;
};

struct UnixIStream: std::istream {
	explicit UnixIStream( int fd, size_t bs = 4096 ):
		std::istream( new UnixStreamBuf( fd, bs ) ) {
	}

	~UnixIStream() {
		delete rdbuf();
	}
};

#if defined( __linux__ )
inline void closefrom( int fd ) {
	int end = getdtablesize(); // XXX: use /proc/{pid}/status
	while( fd < end ) {
		close( fd );
		++fd;
	}
}
#endif

template<class Container>
int forkExec( Container const& args, int ifd, int ofd ) {
	assert( args.size() >= 1 );

	char const** args_raw = new char const*[args.size() + 1];
	for( size_t i = 0; i < args.size(); ++i ) {
		args_raw[i] = args[i].c_str();
	}
	args_raw[args.size()] = NULL;

	pid_t pid = fork();
	checkSysCall( pid );
	if( pid == 0 ) {
		dup2( ifd, 0 );
		dup2( ofd, 1 );
		closefrom( 3 );
		execvp( args_raw[0], const_cast<char* const*>( args_raw ) );
		_exit( 1 );
	}

	int status;
	checkSysCall( waitpid( pid, &status, 0 ) );
	return WEXITSTATUS( status );
}

struct Thread {
	struct Interrupt {
	};

	static void setup() {
		_interrupted() = false;

		struct sigaction sa;
		memset( &sa, 0, sizeof( sa ) );
		sa.sa_flags = 0;
		sa.sa_handler = _sigHandler;
		checkSysCall( sigaction( SIGUSR1, &sa, NULL ) );
	}

	static void checkIntr() {
		if( _interrupted() ) {
			throw Interrupt();
		}
	}

	template<class T>
	explicit Thread( T const& cb ):
		_callback( new function<void ()>( cb ) ) {
		if( pthread_create( &_thread, NULL, _wrap, _callback ) != 0 ) {
			delete _callback;
			throw IOError();
		}

		sigset_t sset;
		sigemptyset( &sset );  
		sigaddset( &sset, SIGUSR1 );  
		if( pthread_sigmask( SIG_SETMASK, &sset, NULL ) != 0 ) {
			delete _callback;
			throw IOError();
		}
	}

	bool join() {
		void* result;
		if( pthread_join( _thread, &result ) != 0 ) {
			throw IOError();
		}
		return bool( reinterpret_cast<uintptr_t>( result ) );
	}

	// This function behaves like Java's Thread.interrupt() rather than boost::thread::interrupt()
	void interrupt() {
		if( pthread_kill( _thread, SIGUSR1 ) != 0 ) {
			throw IOError();
		}
	}

	private:
		static void _sigHandler( int sig ) {
			if( sig == SIGUSR1 ) {
				_interrupted() = true;
			}
		}

		static void* _wrap( void* data ) {
			_interrupted() = false;
			function<void ()>* callback = reinterpret_cast<function<void ()>*>( data );
			try {
				(*callback)();
				delete callback;
				return reinterpret_cast<void*>( true );
			}
			catch( ... ) {
				delete callback;
				return reinterpret_cast<void*>( false );
			}
		}

	public: static bool volatile& _interrupted() {
			static __thread bool volatile intr;
			return intr;
		}
	private:

		pthread_t _thread;
		function<void ()>* const _callback;
};

inline int checkSysCall( int retv ) {
	if( retv < 0 ) {
		if( errno == EINTR ) {
			throw Thread::Interrupt();
		}
		else {
			throw IOError();
		}
	}
	return retv;
}

inline void writeAll( int ofd, string const& src ) {
	size_t i = 0;
	while( i < src.size() ) {
		i += checkSysCall( write( ofd, src.data() + i, src.size() - i ) );
	}
}
