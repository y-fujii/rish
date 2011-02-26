// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include "config.hpp"
#include <algorithm>
#include <vector>
#include <iostream>
#include <tr1/functional>
#include <stdint.h>
#include <cassert>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>


#define MATCH( c ) break; case (c):
#define OTHERWISE break; default:

namespace std {
	using namespace tr1;
	using namespace tr1::placeholders;

	template<class SrcIter, class DstIter, class Pred>
	inline DstIter copy_if( SrcIter srcIt, SrcIter srcEnd, DstIter dstIt, Pred pred ) {
		while( srcIt != srcEnd ) {
			if( pred( *srcIt ) ) {
				*dstIt++ = *srcIt;
			}
			++srcIt;
		}
		return dstIt;
	}

	template<class Iter, class Func>
	inline bool any_of( Iter bgn, Iter end, Func f ) {
		return std::find_if( bgn, end, f ) != end;
	}

	template<class Iter, class Func>
	inline bool all_of( Iter bgn, Iter end, Func f ) {
		return std::find_if( bgn, end, std::not1( f ) ) == end;
	}

	template<class T>
	struct atomic {
		explicit atomic( T const& v ): _val( v ) {
		}

		T load() volatile {
			__sync_synchronize();
			T volatile v = _val;
			__sync_synchronize();
			return v;
		}

		void store( T const& v ) volatile {
			__sync_synchronize();
			_val = v;
			__sync_synchronize();
		}

		private:
			T volatile _val;
	};
}

#if !defined( __GNUC__ )
struct {
	template<class T>
	operator T*() const {
		return 0;
	}
} const nullptr = {};
#else
#define nullptr 0
#endif

template<class T, unsigned N>
unsigned size( T const (&)[N] ) {
	return N;
}

struct ScopeExit {
	template<class T>
	ScopeExit( T const& cb ): _callback( cb ) {
	}

	~ScopeExit() {
		_callback();
	}

	private:
		std::function<void ()> const _callback;
};

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
			setg( nullptr, nullptr, 0 );
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

struct Thread {
	template<class T>
	explicit Thread( T const& cb ):
		_callback( cb ) {
		if( pthread_create( &_thread, NULL, _wrap, this ) != 0 ) {
			throw std::exception();
		}
	}

	// XXX
	// never called from own thread
	~Thread() {
		try {
			kill( SIGINT );
			join(); // XXX
		}
		catch( ... ) {
			try { kill( SIGKILL ); } catch ( ... ) {}
		}
	}

	bool join() {
		void* result;
		if( pthread_join( _thread, &result ) != 0 ) {
			throw std::exception();
		}
		return bool( reinterpret_cast<uintptr_t>( result ) );
	}

	void kill( int sig ) {
		if( pthread_kill( _thread, sig ) != 0 ) {
			throw std::exception();
		}
	}

	private:
		static void* _wrap( void* self ) {
			try {
				reinterpret_cast<Thread*>( self )->_callback();
			}
			catch( ... ) {
				return reinterpret_cast<void*>( false );
			}
			return reinterpret_cast<void*>( true );
		}

		pthread_t _thread;
		std::function<void ()> const _callback;
};
