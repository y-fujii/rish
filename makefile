LEX = flex -X
YACC = byacc
CXX = g++ -g -pedantic -Wall -Wextra \
	  -I$(HOME)/usr/include -L$(HOME)/usr/lib

SRCS = lexer.l parser.y parser.hpp misc.hpp ast.hpp eval.hpp command.hpp wildcard.hpp main.cpp

rish: $(SRCS)
	$(YACC) -dv parser.y; \
	mv y.tab.h tokens.hpp; \
	mv y.tab.c parser.cpp; \
	mv y.output parser.out
	$(LEX) -o lexer.cpp --header-file=lexer.hpp lexer.l
	$(CXX) -o rish main.cpp lexer.cpp parser.cpp -ledit -lcurses

wc:
	wc $(SRCS)

clean:
	rm -f tokens.hpp parser.cpp parser.out lexer.hpp lexer.cpp rish
