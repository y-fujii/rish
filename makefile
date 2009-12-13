LEX = flex -X
YACC = byacc

all:
	$(YACC) -d -o parse.cpp parse.y
	mv parse.h parse.hpp
	$(LEX) -o lex.cpp lex.l

clean:
	rm -f parse.hpp parse.cpp lex.cpp
