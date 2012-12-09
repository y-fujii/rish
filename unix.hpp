// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <cassert>
#include <iomanip>
#include <map>
#include <sstream>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
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
			throw system_error( errno, system_category() );
		}
	}

	static void checkIntr() {
		if( _interrupted() ) {
			_interrupted() = false;
			throw Interrupt();
		}
	}

	static void interrupt( thread& t ) {
		if( int ec = pthread_kill( t.native_handle(), SIGUSR1 ) ) {
			throw system_error( ec, system_category() );
		}
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

inline int checkSysCall( int retv ) {
	if( retv < 0 ) {
		if( errno != EINTR ) {
			throw system_error( errno, system_category() );
		}
		if( ThreadSupport::_interrupted() ) {
			ThreadSupport::_interrupted() = false;
			throw ThreadSupport::Interrupt();
		}
	}
	return retv;
}

struct UnixStreamBuf: streambuf {
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
		vector<char> _buf;
};

struct UnixIStream: istream {
	explicit UnixIStream( int fd, size_t bs = PIPE_BUF ):
		istream( new UnixStreamBuf( fd, bs ) ) {
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

template<class Iter>
pid_t forkExec( Iter argsB, Iter argsE, int ifd, int ofd ) {
	size_t size = distance( argsB, argsE );
	assert( size >= 1 );

	vector<char const*> argsRaw( size + 1 );
	for( size_t i = 0; i < size; ++i ) {
		argsRaw[i] = argsB[i].c_str();
	}
	argsRaw[size] = NULL;

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

	void put( unique_ptr<T>&& m ) {
		T* p = m.release();
		checkSysCall( write( _ofd, &p, sizeof( &p ) ) );
	}

	unique_ptr<T> get() {
		T* p;
		checkSysCall( read( _ifd, &p, sizeof( &p ) ) );
		return unique_ptr<T>( p );
	}

	bool empty() {
		pollfd pfd{ _ifd, POLLIN, 0 };
		checkSysCall( poll( &pfd, 1, 0 ) );
		return (pfd.revents & POLLIN) != 0;
	}

	int fd() const {
		return _ifd;
	}

	private:
		int _ifd, _ofd;
};

struct EventLooper {
	EventLooper() {
		if( !_signalFd().init ) {
			int fds[2];
			checkSysCall( pipe( fds ) );
			checkSysCall( fcntl( fds[0], F_SETFL, O_NONBLOCK ) );
			checkSysCall( fcntl( fds[1], F_SETFL, O_NONBLOCK ) );
			_signalFd() = { true, fds[0], fds[1] };
		}
	}

	void addReader( int fd, function<void ()> cb ) {
		_handlers[fd].reader = move( cb );
	}

	void addWriter( int fd, function<void ()> cb ) {
		_handlers[fd].writer = move( cb );
	}

	void addSignal( int sig, function<void ()> cb ) {
		struct sigaction sa;
		memset( &sa, 0, sizeof( sa ) );
		sa.sa_flags = SA_RESTART;
		sa.sa_handler = _handleSignal;
		checkSysCall( sigaction( sig, &sa, nullptr ) );

		sigset_t mask;
		checkSysCall( sigemptyset( &mask ) );
		checkSysCall( sigaddset( &mask, sig ) );
		if( int ec = pthread_sigmask( SIG_UNBLOCK, &mask, nullptr ) ) {
			throw system_error( ec, system_category() );
		}

		_signals[sig] = move( cb );
	}

	void removeReader( int fd ) {
		if( _handlers.at( fd ).writer ) {
			_handlers[fd].reader = {};
		}
		else {
			_handlers.erase( fd );
		}
	}

	void removeWriter( int fd ) {
		if( _handlers.at( fd ).reader ) {
			_handlers[fd].writer = {};
		}
		else {
			_handlers.erase( fd );
		}
	}

	void removeSignal( int sig ) {
		struct sigaction sa;
		memset( &sa, 0, sizeof( sa ) );
		sa.sa_handler = SIG_DFL;
		checkSysCall( sigaction( sig, &sa, nullptr ) );

		_signals.erase( sig );
	}

	// must be safe to call add*(), remove*() from handlers
	void wait() {
		vector<pollfd> pfds;
		pfds.emplace_back( pollfd{ _signalFd().ifd, POLLIN, 0 } );
		for( auto const& h: _handlers ) {
			short ev = 0;
			if( h.second.reader ) {
				ev |= POLLIN;
			}
			if( h.second.writer ) {
				ev |= POLLOUT;
			}
			pfds.emplace_back( pollfd{ h.first, ev, 0 } );
		}

		checkSysCall( poll( pfds.data(), pfds.size(), -1 ) );

		if( pfds[0].revents & POLLIN ) {
			uint8_t buf[PIPE_BUF];
			size_t n = checkSysCall( read( _signalFd().ifd, buf, PIPE_BUF ) );
			for( size_t i = 0; i < n; ++i ) {
				auto it = _signals.find( buf[i] );
				if( it != _signals.end() ) {
					it->second();
				}
			}
		}

		for( auto pit = pfds.begin() + 1; pit < pfds.end(); ++pit ) {
			if( pit->revents & POLLIN ) {
				auto hit = _handlers.find( pit->fd );
				if( hit != _handlers.end() && hit->second.reader ) {
					hit->second.reader();
					// _handlers may be changed here
				}
			}
			if( pit->revents & POLLOUT ) {
				auto hit = _handlers.find( pit->fd );
				if( hit != _handlers.end() && hit->second.writer ) {
					hit->second.writer();
					// _handlers may be changed here
				}
			}
		}
	}

	private:
		struct FdHandler {
			function<void ()> reader;
			function<void ()> writer;
		};

		struct SignalFd {
			volatile bool init;
			volatile int ifd;
			volatile int ofd;
		};

		static SignalFd& _signalFd() {
			static SignalFd sigFd{ false, -1, -1 };
			return sigFd;
		}

		static void _handleSignal( int sigI ) {
			uint8_t sigB = sigI;
			if( write( _signalFd().ofd, &sigB, 1 ) < 0 ) {
				terminate();
			}
		}

		map<int, FdHandler> _handlers;
		map<int, function<void ()>> _signals;
};
