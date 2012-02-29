#CXX = clang++ -std=c++11 -stdlib=libc++ -pedantic -Wall -Wextra -fno-rtti -pthread -O3 -s -I$(HOME)/usr/src/libcxx/include
CXX = g++46 -std=c++0x -pedantic -Wall -Wextra -fno-rtti -pthread -O3 -s
LEX = flex

SRCS = \
	lexer.l parser.y \
	misc.hpp unix.hpp \
	parser.hpp ast.hpp eval.hpp glob.hpp \
	builtins.hpp main.cpp

rish: $(SRCS)
	$(YACC) -dv parser.y; \
	mv y.tab.h tokens.hpp; \
	mv y.tab.c parser.cpp; \
	mv y.output parser.out
	$(LEX) lexer.l; \
	mv lex.yy.c lexer.cpp
	$(CXX) -o rish main.cpp lexer.cpp parser.cpp -lreadline -lcurses

test: rish
	wc $(SRCS)
	./rish test.rs

clean:
	rm -f tokens.hpp parser.cpp parser.out lexer.cpp rish
