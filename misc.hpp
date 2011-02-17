#pragma once

#include <algorithm>
#include <iostream>
#include <tr1/functional>
#include <cassert>
#include <unistd.h>
#include <pthread.h>


#define ARR_SIZE( arr ) (sizeof( (arr) ) / sizeof( (arr)[0] ))
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
}

struct UnixStreamBuf: std::streambuf {
	static int const bufSize = 4096;

	UnixStreamBuf( int fd ): _fd( fd ) {
	}

	virtual int underflow() {
		ssize_t n = read( _fd, _buf, bufSize );
		if( n < 0 ) {
			throw std::ios_base::failure( "read()" );
		}
		else if( n == 0 ) {
			return traits_type::eof();
		}
		else /* n > 0 */ {
			setg( _buf, _buf, _buf + n );
			return _buf[0];
		}
	}

	private:
		int _fd;
		char _buf[bufSize];
};

struct UnixIStream: std::istream {
	UnixIStream( int fd ): std::istream( new UnixStreamBuf( fd ) ) {
	}

	~UnixIStream() {
		delete rdbuf();
	}
};

struct Thread {
	Thread( std::function<void ()> const& cb ):
		_callback( cb ) {
		if( pthread_create( &_thread, NULL, _wrap, this ) != 0 ) {
			throw std::exception();
		}
	}

	void join() {
		if( pthread_join( _thread, NULL ) != 0 ) {
			throw std::exception();
		}
	}

	private:
		static void* _wrap( void* self ) {
			reinterpret_cast<Thread*>( self )->_callback();
			return 0;
		}

		pthread_t _thread;
		std::function<void ()> /* const& */ _callback;
};
