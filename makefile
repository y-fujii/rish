#CXX  = clang++
#LEX  = flex
#YACC = yacc
OPTS = -std=gnu++14 -pedantic -Wall -Wextra -pthread -O3
SRCS = src/lexer.l src/parser.y src/main.cpp $(wildcard src/*.hpp)

.PHONY: all clean test analyze

all: build/rish build/str

clean:
	rm -rf build

test: build/rish
	wc $(SRCS)
	build/rish examples/test.rs

analyze: $(SRCS)
	mkdir -p build
	$(LEX)  -obuild/lexer.cpp  src/lexer.l
	$(YACC) -obuild/parser.cpp src/parser.y
	cd build && clang++ --analyze $(OPTS) ../src/main.cpp

build/rish: $(SRCS) makefile
	mkdir -p build
	$(LEX)  -obuild/lexer.cpp  src/lexer.l
	$(YACC) -obuild/parser.cpp src/parser.y
	$(CXX) $(OPTS) -o build/rish src/main.cpp -lreadline

build/str: src/cmd_str.cpp makefile
	mkdir -p build
	$(CXX) $(OPTS) -o build/str src/cmd_str.cpp
