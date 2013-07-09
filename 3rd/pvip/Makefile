CFLAGS=-Wall -DYY_DEBUG=1 -g

all: pvip

pvip: libpvip.a main.c
	gcc $(CFLAGS) -L. -lpvip -o pvip main.c

libpvip.a: gen.pvip.y.o pvip.h pvip_node.o gen.node.o gen.node.o
	ar rsv libpvip.a gen.pvip.y.o pvip_node.o gen.node.o

test: pvip
	prove -lr t

.c.o:
	cc $(CFLAGS) -c $<

gen.node.c: build/node.pl
	perl build/node.pl

gen.pvip.y.c: pvip.y pvip.h gen.node.c
	../greg/greg -o gen.pvip.y.c pvip.y

clean:
	rm -f *.o gen.* pvip

.PHONY: all test

