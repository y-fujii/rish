#CXX = clang++ -g -pedantic -Wall -Wextra -pthread -I/usr/include/boost/tr1 -DBOOST_TR1_GCC_INCLUDE_PATH=4.5.2
CXX = g++ -g -pedantic -Wall -Wextra -pthread -O3

SRCS = \
	lexer.l parser.y \
	misc.hpp unix.hpp \
	parser.hpp ast.hpp eval.hpp glob.hpp \
	builtins.hpp main.cpp

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
