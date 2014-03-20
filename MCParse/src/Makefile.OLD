
# $Id$

all: revno libmcparse.a

clean:
	touch mcparse.o libmcparse.a
	rm *.o libmcparse.a

install:
	cp libmcparse.a ../lib
	cp libmcparse.a ../../lib
	cp mcparse.h ../include
	cp mcparse.h ../../include

revno:
	./BuildRevNo

revision.h:
	./BuildRevNo

mcparse.o: mcparse.h mcparse.cc revision.h
	g++ $(CXXFLAGS) -c mcparse.cc 

libmcparse.a: mcparse.o 
	ar -r libmcparse.a mcparse.o 
	ranlib libmcparse.a

