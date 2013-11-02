
# $Id$

all: revno librng.a

install:
	cp librng.a ../lib
	cp rng.h ../include
	cp librng.a ../../../lib
	cp rng.h ../../../include

clean:
	touch rng.o librng.a; rm *.o librng.a

revno:
	./BuildRevNo

revision.h:
	./BuildRevNo

rng.o: rng.h rng.cc revision.h
	g++ $(CXXFLAGS) -c rng.cc 

librng.a: rng.o 
	ar -r librng.a rng.o 
	ranlib librng.a

