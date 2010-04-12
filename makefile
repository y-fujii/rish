LEX = flex -X
YACC = byacc
CXX = g++44 -pedantic -Wall -Wextra

rish: lexer.l parser.y ast.hpp eval.hpp main.cpp
	$(YACC) -dv parser.y; \
	mv y.tab.h tokens.hpp; \
	mv y.tab.c parser.cpp; \
	mv y.output parser.out
	$(LEX) -o lexer.cpp --header-file=lexer.hpp lexer.l
	$(CXX) -o rish main.cpp lexer.cpp parser.cpp -ledit

wc:
	wc parser.y lexer.l ast.hpp eval.hpp main.cpp

clean:
	rm -f tokens.hpp parser.cpp parser.out lexer.hpp lexer.cpp rish
