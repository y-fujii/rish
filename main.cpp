
#include <histedit.h>


int main( int argc, char** argv ) {
	FILE* stdin = fdopen(0, "r");
	FILE* stderr = fdopen(1, "w");
	FILE* stdout = fdopen(2, "w");

	EditLine* el = el_init( argv[0], stdin, stdout, stderr );
	el_gets( el, );

	return 0;
}
