LEX = flex -X
YACC = byacc
CXX = g++ -pedantic -Wall -Wextra

all:
	$(YACC) -d parse.y
	mv y.tab.h parse.hpp
	mv y.tab.c parse.cpp
	$(LEX) -o lex.cpp lex.l
	$(CXX) -o rish main.cpp -ledit

clean:
	rm -f parse.hpp parse.cpp lex.cpp rish
