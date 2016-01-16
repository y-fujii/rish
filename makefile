CXX  = g++
#CXX  = clang++
LEX  = flex
YACC = byacc
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

analyze: lexer.cpp parser.cpp
	clang++ --analyze $(OPTS) -include pch.hpp main.cpp

clean:
	rm -f tokens.hpp parser.cpp parser.out lexer.cpp pch.hpp.gch rish str

rish: $(SRCS) lexer.cpp parser.cpp makefile pch.hpp.gch
	$(CXX) $(OPTS) -include pch.hpp -o rish main.cpp -lreadline

lexer.cpp: lexer.l
	$(LEX) -olexer.cpp lexer.l

parser.cpp: parser.y
	$(YACC) -dv parser.y; \
	mv y.tab.h tokens.hpp; \
	mv y.tab.c parser.cpp; \
	mv y.output parser.out

str: cmd_str.cpp makefile pch.hpp.gch
	$(CXX) $(OPTS) -include pch.hpp -o str cmd_str.cpp

pch.hpp.gch: pch.hpp makefile
	$(CXX) $(OPTS) -x c++-header pch.hpp
