#CXX = g++
CXX = clang++
LEX = flex
OPTS = -std=c++14 -pedantic -Wall -Wextra -pthread -O3

SRCS = \
	lexer.l parser.y \
	misc.hpp unix.hpp \
	parser.hpp ast.hpp annotate.hpp eval.hpp glob.hpp \
	builtins.hpp repl.hpp main.cpp

.PHONY: all test analyze clean
all: rish str

test: rish
	wc $(SRCS)
	./rish test.rs

analyze:
	clang++ --analyze $(OPTS) main.cpp lexer.cpp parser.cpp

clean:
	rm -f tokens.hpp parser.cpp parser.out lexer.cpp rish str

rish: $(SRCS) makefile
	$(YACC) -dv parser.y; \
	mv y.tab.h tokens.hpp; \
	mv y.tab.c parser.cpp; \
	mv y.output parser.out
	$(LEX) -olexer.cpp lexer.l; \
	$(CXX) $(OPTS) -o rish main.cpp lexer.cpp parser.cpp -lreadline

str: cmd_str.cpp makefile
	$(CXX) $(OPTS) -o str cmd_str.cpp
