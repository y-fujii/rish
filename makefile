CXX = clang++ -std=c++11 -pedantic -Wall -Wextra -stdlib=libc++ -lc++abi -pthread -g -O3
#CXX = g++ -std=c++11 -pedantic -Wall -Wextra -pthread -g -O0
LEX = flex

SRCS = \
	lexer.l parser.y \
	misc.hpp unix.hpp \
	parser.hpp ast.hpp annotate.hpp eval.hpp glob.hpp \
	builtins.hpp repl.hpp main.cpp

.PHONY: all
all: rish str

rish: $(SRCS) makefile
	$(YACC) -dv parser.y; \
	mv y.tab.h tokens.hpp; \
	mv y.tab.c parser.cpp; \
	mv y.output parser.out
	$(LEX) -olexer.cpp lexer.l; \
	$(CXX) -orish main.cpp lexer.cpp parser.cpp -lreadline

str: cmd_str.cpp makefile
	$(CXX) -ostr cmd_str.cpp

test: rish
	wc $(SRCS)
	./rish test.rs

clean:
	rm -f tokens.hpp parser.cpp parser.out lexer.cpp rish str
