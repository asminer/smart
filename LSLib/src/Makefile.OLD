
# $Id$

all: revno libls.a

clean:
	touch lslib.o libls.a
	rm *.o libls.a

install:
	cp libls.a ../lib
	cp lslib.h ../include
	cp libls.a ../../lib
	cp lslib.h ../../include

revno:
	./BuildRevNo

revision.h:
	./BuildRevNo

lslib.o: lslib.h lslib.cc revision.h
	g++ $(CXXFLAGS) -c lslib.cc 

libls.a: lslib.o 
	ar -r libls.a lslib.o 
	ranlib libls.a

