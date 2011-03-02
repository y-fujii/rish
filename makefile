LEX = flex -X
CXX = clang++ -g -pedantic -Wall -Wextra -pthread -I/usr/include/boost/tr1
#CXX = g++ -g -pedantic -Wall -Wextra -pthread

SRCS = \
	lexer.l parser.y \
	config.hpp misc.hpp exception.hpp \
	parser.hpp ast.hpp eval.hpp command.hpp glob.hpp \
	main.cpp

rish: $(SRCS)
	$(YACC) -dv parser.y; \
	mv y.tab.h tokens.hpp; \
	mv y.tab.c parser.cpp; \
	mv y.output parser.out
	$(LEX) lexer.l; \
	mv lex.yy.c lexer.cpp
	$(CXX) -o rish main.cpp lexer.cpp parser.cpp -lreadline -lcurses

test: rish
	wc $(SRCS)
	./rish

clean:
	rm -f tokens.hpp parser.cpp parser.out lexer.cpp rish
