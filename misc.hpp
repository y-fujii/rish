#pragma once

#include "config.hpp"
#include <algorithm>
#include <vector>
#include <iostream>
#include <tr1/functional>
#include <cassert>
#include <unistd.h>
#include <pthread.h>


#define ARR_SIZE( arr ) (sizeof( arr ) / sizeof( (arr)[0] ))
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

	struct {
		template<class T>
		operator T*() const {
			return 0;
		}
	} const nullptr = {};
}

struct UnixStreamBuf: std::streambuf {
	UnixStreamBuf( int fd, size_t bs ):
		_fd( fd ),
		_buf( bs ) {
	}

	virtual int underflow() {
		ssize_t n = read( _fd, &_buf[0], _buf.size() );
		if( n < 0 ) {
			throw std::ios_base::failure( "read()" );
		}
		else if( n == 0 ) {
			return traits_type::eof();
		}
		else /* n > 0 */ {
			setg( &_buf[0], &_buf[0], &_buf[0] + _buf.size() );
			return _buf[0];
		}
	}

	private:
		int const _fd;
		std::vector<char> _buf;
};

struct UnixIStream: std::istream {
	UnixIStream( int fd, size_t bs = 4096 ):
		std::istream( new UnixStreamBuf( fd, bs ) ) {
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
		std::function<void ()> const _callback;
};
