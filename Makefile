all: nqp-parser

test: nqp-parser
	prove -r t

clean:
	rm -f nqp-parser nqp-parser.cc

nqp-parser: nqp-parser.cc node.h
	clang++ -g -std=c++11 -Wall -Werror -o nqp-parser nqp-parser.cc

nqp-parser.cc: nqp.y
	greg -o nqp-parser.cc nqp.y

node.h: build/nqpc-node.pl
	perl build/nqpc-node.pl > node.h

.PHONY: all clean test
