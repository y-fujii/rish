LEX = flex -X
#YACC = byacc
#CXX = g++ -g -pedantic -Wall -Wextra
CXX = clang++ -g -pedantic -Wall -Wextra

SRCS = lexer.l parser.y parser.hpp misc.hpp ast.hpp eval.hpp command.hpp glob.hpp main.cpp

rish: $(SRCS)
	$(YACC) -dv parser.y; \
	mv y.tab.h tokens.hpp; \
	mv y.tab.c parser.cpp; \
	mv y.output parser.out
	$(LEX) -o lexer.cpp --header-file=lexer.hpp lexer.l
	$(CXX) -o rish main.cpp lexer.cpp parser.cpp -lreadline -lcurses

test: rish
	wc $(SRCS)
	./rish

clean:
	rm -f tokens.hpp parser.cpp parser.out lexer.hpp lexer.cpp rish
