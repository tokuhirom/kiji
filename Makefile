all: nqp-parser

test: nqp-parser
	prove -r t

clean:
	rm -f nqp-parser nqp-parser.cc

nqp-parser: src/gen.nqp.y.cc src/gen.node.h
	clang++ -g -std=c++11 -Wall -o nqp-parser src/nqp-parser.cc

src/gen.nqp.y.cc: src/nqp.y
	greg -o src/gen.nqp.y.cc src/nqp.y

src/gen.node.h: build/nqpc-node.pl
	perl build/nqpc-node.pl > src/gen.node.h

.PHONY: all clean test
