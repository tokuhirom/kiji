all: _build/saru-parser

test: _build/saru-parser
	prove -r t

clean:
	rm -rf _build/ saru-parser.cc

_build/saru-parser: src/gen.saru.y.cc src/gen.node.h
	mkdir -p _build/
	clang++ -g -std=c++11 -Wall -o _build/saru-parser src/saru-parser.cc

src/gen.saru.y.cc: src/saru.y
	greg -o src/gen.saru.y.cc src/saru.y

src/gen.node.h: build/saru-node.pl
	perl build/saru-node.pl > src/gen.node.h

.PHONY: all clean test
