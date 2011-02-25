// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once


#if defined( __clang__ ) && defined( __linux__ )
#define BOOST_TR1_GCC_INCLUDE_PATH 4.5.2
#endif

#if defined( __linux__ )
#include <unistd.h>
inline void closefrom( int fd ) {
	int end = getdtablesize(); // XXX: use /proc/{pid}/status
	while( fd < end ) {
		close( fd );
		++fd;
	}
}
#endif

#define MEMORY_BARRIOR __sync_synchronize();
