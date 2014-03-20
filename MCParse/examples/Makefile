
# $Id$

all: parsetest renumber

INCPATH=-I../src -I../../GraphLib/include
LIBPATH=-L../src -L../../GraphLib/lib -L../../IntSets/lib

GENFILES= parsetest parsetest.o renumber renumber.o

clean:
	touch $(GENFILES); rm $(GENFILES)


parsetest: parsetest.o ../src/libmcparse.a
	g++ $(CXXFLAGS) parsetest.o $(LIBPATH) -lmcparse -o parsetest

parsetest.o: parsetest.cc ../src/mcparse.h
	g++ $(CXXFLAGS) $(INCPATH) -c parsetest.cc

renumber: renumber.o ../src/libmcparse.a
	g++ $(CXXFLAGS) renumber.o $(LIBPATH) -lmcparse -lgraph -lintset -o renumber

renumber.o: renumber.cc ../src/mcparse.h
	g++ $(CXXFLAGS) $(INCPATH) -c renumber.cc
