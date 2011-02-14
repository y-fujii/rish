#pragma once


#if defined( __clang__ )
#define BOOST_TR1_GCC_INCLUDE_PATH 4.5.2
#endif

#if !defined( HAVE_CLOSEFROM )
inline void closefrom( int fd ) {
	int end = getdtablesize();
	while( fd < end ) {
		close( fd );
		++fd;
	}
}
#endif
