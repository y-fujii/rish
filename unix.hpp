// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <deque>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <sys/select.h>
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
pid_t forkExec( Container const& args, int ifd, int ofd ) {
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

	return pid;
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
			this->~Thread();
			throw IOError();
		}
	}

	explicit Thread( string const& name ):
		_callback( nullptr ),
		_detach( true )
	{
		static_assert( sizeof( pthread_t ) == sizeof( uintptr_t ), "" );

		istringstream ifs( name );
		ifs.exceptions( ios_base::failbit | ios_base::badbit );
		char tee;
		ifs >> tee;
		if( tee != 'T' ) {
			throw IOError();
		}
		ifs >> hex >> (uintptr_t&)( _thread );
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
		pthread_detach( _thread );
		_detach = true;
	}

	string name() const {
		static_assert( sizeof( pthread_t ) == sizeof( uintptr_t ), "" );

		ostringstream ofs;
		ofs.exceptions( ios_base::failbit | ios_base::badbit );
		ofs << 'T';
		ofs << hex << setw( sizeof( pthread_t ) * 2 ) << setfill( '0' ) << uintptr_t( _thread );
		return ofs.str();
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

struct Selector {
	Selector( initializer_list<int> ifds, initializer_list<int> ofds ) {
		FD_ZERO( &_result_ifds );
		FD_ZERO( &_result_ofds );

		FD_ZERO( &_ifds );
		for( int fd: ifds ) {
			FD_SET( fd, &_ifds );
		}

		FD_ZERO( &_ofds );
		for( int fd: ofds ) {
			FD_SET( fd, &_ofds );
		}

		int imax = *max_element( ifds.begin(), ifds.end() );
		int omax = *max_element( ofds.begin(), ofds.end() );
		_maxFd = max( imax, omax );
	}

	void wait() {
		memcpy( &_result_ifds, &_ifds, sizeof( fd_set ) );
		memcpy( &_result_ofds, &_ofds, sizeof( fd_set ) );
		checkSysCall( select( _maxFd, &_result_ifds, &_result_ofds, nullptr, nullptr ) );
	}

	bool readable( int fd ) const {
		return FD_ISSET( fd, &_result_ifds );
	}

	bool writable( int fd ) const {
		return FD_ISSET( fd, &_result_ofds );
	}

	private:
		fd_set _ifds;
		fd_set _ofds;
		fd_set _result_ifds;
		fd_set _result_ofds;
		int _maxFd;
};

inline bool readable( int fd ) {
	fd_set fds;
	FD_ZERO( &fds );
	FD_SET( fd, &fds );

	struct timeval timeout;
	timeout.tv_sec  = 0;
	timeout.tv_usec = 0;

	checkSysCall( select( fd, &fds, nullptr, nullptr, &timeout ) );

	return FD_ISSET( fd, &fds );
}

inline bool writable( int fd ) {
	fd_set fds;
	FD_ZERO( &fds );
	FD_SET( fd, &fds );

	struct timeval timeout;
	timeout.tv_sec  = 0;
	timeout.tv_usec = 0;

	checkSysCall( select( fd, nullptr, &fds, nullptr, &timeout ) );

	return FD_ISSET( fd, &fds );
}

template<class T>
struct MsgQueue {
	// guarantee atomicity
	static_assert( PIPE_BUF >= sizeof( T* ), "" );

	~MsgQueue() {
		close( _ofd );

		T* p;
		while( read( _ifd, &p, sizeof( &p ) ) == sizeof( &p ) ) {
			delete p;
		}
		close( _ifd );
	}

	MsgQueue() {
		int fds[2];
		checkSysCall( pipe( fds ) );
		_ifd = fds[0];
		_ofd = fds[1];
	}

	void put( std::unique_ptr<T>&& m ) {
		T* p = m.release();
		checkSysCall( write( _ofd, &p, sizeof( &p ) ) );
	}

	std::unique_ptr<T> get() {
		T* p;
		checkSysCall( read( _ifd, &p, sizeof( &p ) ) );
		return std::unique_ptr<T>( p );
	}

	bool empty() {
		return readable( _ifd );
	}

	int fd() const {
		return _ifd;
	}

	private:
		int _ifd, _ofd;
};
