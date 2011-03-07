// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <deque>
#include <cassert>
#include <sstream>
#include <unistd.h>
#include "misc.hpp"
#include "glob.hpp"

using namespace std;


struct UnixStreamBuf: std::streambuf {
	UnixStreamBuf( int fd, size_t bs ):
		_fd( fd ), _buf( bs ) {
	}

	virtual int underflow() {
		ssize_t n = read( _fd, &_buf[0], _buf.size() );
		if( n < 0 ) {
			throw std::ios_base::failure( "read()" );
		}
		else if( n == 0 ) {
			setg( nullptr, nullptr, nullptr );
			return traits_type::eof();
		}
		else /* n > 0 */ {
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
	if( pid < 0 ) {
		throw IOError();
	}
	else if( pid == 0 ) {
		dup2( ifd, 0 );
		dup2( ofd, 1 );
		closefrom( 3 );
		execvp( args_raw[0], const_cast<char* const*>( args_raw ) );
		_exit( 1 );
	}

	int status;
	if( waitpid( pid, &status, 0 ) < 0 ) {
		throw IOError();
	}
	return WEXITSTATUS( status );
}

struct Thread {
	struct Interrupt {
	};

	template<class T>
	explicit Thread( T const& cb ):
		_callback( new function<void ()>( cb ) ) {
		if( pthread_create( &_thread, NULL, _wrap, _callback ) != 0 ) {
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

	void interrupt() {
		if( pthread_kill( _thread, SIGINT ) != 0 ) {
			throw IOError();
		}
	}

	private:
		static void* _wrap( void* data ) {
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

		pthread_t _thread;
		std::function<void ()>* const _callback;
};

inline void checkSysCall( int retv ) {
	if( retv < 0 ) {
		if( errno == EINTR ) {
			throw Thread::Interrupt();
		}
		else {
			throw IOError();
		}
	}
}
