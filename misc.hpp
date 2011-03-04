// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <algorithm>
#include <vector>
#include <iostream>
#include <tr1/functional>
#include <stdint.h>
#include <cassert>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>


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
		return find_if( bgn, end, f ) != end;
	}

	template<class Iter, class Func>
	inline bool all_of( Iter bgn, Iter end, Func f ) {
		return find_if( bgn, end, not1( f ) ) == end;
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

/* this does not work on gcc-4.5.x
struct {
	template<class T>
	operator T*() const {
		return 0;
	}
} const nullptr = {};
*/
#define nullptr (0)

template<class T, unsigned N>
unsigned size( T const (&)[N] ) {
	return N;
}

struct ScopeExit {
	template<class T>
	explicit ScopeExit( T const& cb, bool r = true ):
		_callback( cb ), _run( r ) {
	}

	~ScopeExit() {
		if( _run ) {
			_callback();
		}
	}

	private:
		std::function<void ()> const _callback;
		bool const _run;
};

template<class T>
struct VariantBase {
	template<class U>
	U* dynCast() {
		if( dynId == U::template VariantBase<T>::staId ) {
			return static_cast<U*>( this );
		}
		else {
			return 0;
		}
	}

	int const dynId;

	protected:
		VariantBase(): dynId( -1 ) {
		}
};

template<class T, int Id>
struct Variant: T {
	Variant() {
		const_cast<int&>( VariantBase<T>::dynId ) = Id;
	}

	enum { staId = Id };
};

template<class T>
struct FalseWrapper {
	FalseWrapper( T const& v ): val( v ) {
	}

	operator bool() const {
		return false;
	}

	T val;
};

#define VARIANT_IF( type, lhs, rhs ) \
	if( type* lhs = (rhs)->dynCast<type>() )

#define VARIANT_SWITCH( type, rhs ) \
	if( FalseWrapper<VariantBase<type>*> _variant_switch_value_ = (rhs) ) {} else \
	switch( _variant_switch_value_.val->dynId ) {

#define VARIANT_CASE( type, lhs ) \
		break; \
	} \
	case type::staId: { \
		type* lhs = static_cast<type*>( _variant_switch_value_.val );

#define VARIANT_CASE_( type ) \
		break; \
	} \
	case type::staId: {

#define VARIANT_DEFAULT \
		break; \
	} \
	default:

struct IOError: std::exception {
};

struct Thread {
	template<class T>
	explicit Thread( T const& cb ):
		_callback( cb ) {
		if( pthread_create( &_thread, NULL, _wrap, this ) != 0 ) {
			throw IOError();
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
			throw IOError();
		}
		return bool( reinterpret_cast<uintptr_t>( result ) );
	}

	void kill( int sig ) {
		if( pthread_kill( _thread, sig ) != 0 ) {
			throw IOError();
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
