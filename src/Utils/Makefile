#
# Makefile for test programs.
#
all: test_init test_splay

clean:
	rm *.o test_init test_splay

CFLAGS= --std=c++11

test_init: test_init.o initializer.o messages.o outstream.o location.o sigman.o strings.o
	g++ $^ -o $@

test_splay: test_splay.o splay.o strings.o
	g++ $^ -o $@

%.o: %.cc
	g++ -c $(CFLAGS) $<

# additional dependencies

test_init.o:	initializer.h messages.h outstream.h ../include/defines.h
initializer.o:	initializer.h messages.h outstream.h ../include/defines.h
messages.o: messages.h outstream.h ../include/defines.h
messages.o: location.h sigman.h
# messages.o: ../Options/optman.h ../Options/options.h

location.o: location.h
sigman.o: sigman.h
strings.o: strings.h ../include/shared.h

splay.o: splay.h
