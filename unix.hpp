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
void closefrom( int lowfd ) {
	assert( lowfd >= 0 );

	DIR* dir = opendir( "/proc/self/fd" );
	if( dir == NULL ) {
		return;
	}
	auto closer = scopeExit( bind( closedir, dir ) );

	while( true ) {
		dirent entry;
		dirent* result;
		if( readdir_r( dir, &entry, &result ) != 0 ) {
			return;
		}
		if( result == NULL ) {
			break;
		}
		if( entry.d_type == DT_DIR ) {
			continue;
		}
		int fd = -1;
		sscanf( entry.d_name, "%d", &fd );
		if( fd >= lowfd ) {
			close( fd );
		}
	}
}
#endif

template<class Container>
int forkExec( Container const& args, int ifd, int ofd ) {
	assert( args.size() >= 1 );

	vector<char const*> argsRaw( args.size() + 1 );
	for( size_t i = 0; i < args.size(); ++i ) {
		argsRaw[i] = args[i].c_str();
	}
	argsRaw[args.size()] = NULL;

	pid_t pid = vfork();
	checkSysCall( pid );
	if( pid == 0 ) {
		if( dup2( ifd, 0 ) < 0 ) {
			_exit( 1 );
		}
		if( dup2( ofd, 1 ) < 0 ) {
			_exit( 1 );
		}
		closefrom( 3 );
		execvp( argsRaw[0], const_cast<char* const*>( &argsRaw[0] ) );
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

	~Thread() {
		if( !_detach ) {
			try {
				interrupt();
				join();
			}
			catch( ... ) {}
		}
	}

	template<class T>
	explicit Thread( T const& cb ):
		_callback( new function<void ()>( cb ) ),
		_detach( false )
	{
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

	void join() {
		void* result;
		if( pthread_join( _thread, &result ) != 0 ) {
			throw IOError();
		}
		if( !bool( reinterpret_cast<uintptr_t>( result ) ) ) {
			throw std::exception();
		}
	}

	// This function behaves like Java's Thread.interrupt() rather than boost::thread::interrupt()
	void interrupt() {
		if( pthread_kill( _thread, SIGUSR1 ) != 0 ) {
			throw IOError();
		}
	}

	void detach() {
		_detach = true;
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
		bool _detach;
};

template<class Func0, class Func1>
void parallel( Func0 const& f0, Func1 const& f1 ) {
	Thread thread( f0 );
	f1();
	thread.join();
}

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
