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


struct ThreadSupport {
	struct Interrupt {
	};

	static void setup() {
		_interrupted() = false;

		struct sigaction sa;
		memset( &sa, 0, sizeof( sa ) );
		sa.sa_flags = 0;
		sa.sa_handler = _sigHandler;
		if( sigaction( SIGUSR1, &sa, NULL ) < 0 ) {
			throw IOError();
		}
	}

	static void checkIntr() {
		if( interrupted() ) {
			_interrupted() = false;
			throw Interrupt();
		}
	}

	// This function behaves like Java's Thread.interrupt() rather than
	// boost::thread::interrupt()
	static void interrupt( thread& t ) {
		if( pthread_kill( t.native_handle(), SIGUSR1 ) != 0 ) {
			throw IOError();
		}
	}

	static bool interrupted() {
		return _interrupted();
	}

	static string name( thread& t ) {
		static_assert( sizeof( pthread_t ) == sizeof( uintptr_t ), "" );

		ostringstream ofs;
		ofs.exceptions( ios_base::failbit | ios_base::badbit );
		ofs << hex << setw( sizeof( pthread_t ) * 2 ) << setfill( '0' ) << uintptr_t( t.native_handle() );
		return ofs.str();
	}

	static pthread_t fromName( string const& name ) {
		static_assert( sizeof( pthread_t ) == sizeof( uintptr_t ), "" );

		istringstream ifs( name );
		ifs.exceptions( ios_base::failbit | ios_base::badbit );
		union {
			pthread_t pth;
			uintptr_t val;
		} u;
		ifs >> hex >> u.val;
		return u.pth;
	}

	friend int checkSysCall( int );

	private:
		static void _sigHandler( int sig ) {
			if( sig == SIGUSR1 ) {
				_interrupted() = true;
			}
		}

		static bool volatile& _interrupted() {
			static __thread bool volatile intr;
			return intr;
		}
};

int checkSysCall( int retv ) {
	if( retv < 0 ) {
		if( errno == EINTR && ThreadSupport::interrupted() ) {
			ThreadSupport::_interrupted() = false;
			throw ThreadSupport::Interrupt();
		}
		else {
			throw IOError();
		}
	}
	return retv;
}

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

inline void writeAll( int ofd, string const& src ) {
	size_t i = 0;
	while( i < src.size() ) {
		i += checkSysCall( write( ofd, src.data() + i, src.size() - i ) );
	}
}

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
