LEX = flex -X
YACC = byacc
CXX = g++44 -g -std=gnu++0x -pedantic -Wall -Wextra \
	  -Wno-unused-function

SRCS = lexer.l parser.y parser.hpp ast.hpp eval.hpp command.hpp main.cpp

rish: $(SRCS)
	$(YACC) -dv parser.y; \
	mv y.tab.h tokens.hpp; \
	mv y.tab.c parser.cpp; \
	mv y.output parser.out
	$(LEX) -o lexer.cpp --header-file=lexer.hpp lexer.l
	$(CXX) -o rish main.cpp lexer.cpp parser.cpp -ledit

wc:
	wc $(SRCS)

clean:
	rm -f tokens.hpp parser.cpp parser.out lexer.hpp lexer.cpp rish
