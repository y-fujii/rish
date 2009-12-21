LEX = flex -X
YACC = byacc
CXX = g++ -pedantic -Wall -Wextra

all:
	$(YACC) -dv parser.y; \
	mv y.tab.h tokens.hpp; \
	mv y.tab.c parser.cpp; \
	mv y.output parser.out
	$(LEX) -o lexer.cpp lexer.l
	$(CXX) -o rish main.cpp -ledit

wc:
	wc parser.y lexer.l ast.hpp eval.hpp main.cpp

clean:
	rm -f tokens.hpp parser.cpp parser.out lexer.cpp rish
