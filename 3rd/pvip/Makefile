CFLAGS=-Wall -DYY_DEBUG=1

all: pvip-test

libpvip.a: gen.pvip.y.o pvip.h pvip_node.o gen.node.o gen.node.o
	ar rsv libpvip.a gen.pvip.y.o pvip_node.o gen.node.o

test: pvip-test
	prove -v ./pvip-test

pvip-test: gen.pvip.y.c pvip.h pvip_node.c gen.node.o libpvip.a test.c
	gcc -L. -lpvip -Wall -g $(CFLAGS) -o pvip-test test.c

.c.o:
	cc $(CFLAGS) -c $<

gen.node.c: build/node.pl
	perl build/node.pl

gen.pvip.y.c: pvip.y pvip.h gen.node.c
	../greg/greg -o gen.pvip.y.c pvip.y

clean:
	rm -f *.o gen.* pvip-test

.PHONY: all test

