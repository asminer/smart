
# $Id$

INCPATH=-I../src -I$(HOME)/PNFront/include
LIBPATH=-L../src -L$(HOME)/PNFront/lib

EXES = enum1 enum2 dbtest pngen statediff testcoll
OBJS = timers.o pngen.o statediff.o testcoll.o

all: $(EXES)

clean:
	touch $(EXES) $(OBJS); rm $(EXES) $(OBJS)

check: $(EXES)
	./CheckScript

# Dependencies

timers.o: timers.h timers.cc
pngen.o: pngen.cc timers.h ../src/statelib.h


# enum1

enum1: enum1.cc ../src/libstate.a timers.o
	g++ $(CXXFLAGS) $(INCPATH) enum1.cc timers.o $(LIBPATH) -lstate -o enum1 
# enum2

enum2: enum2.cc ../src/libstate.a timers.h
	g++ $(CXXFLAGS) $(INCPATH) enum2.cc timers.o $(LIBPATH) -lstate -o enum2

# dbtest

dbtest: dbtest.cc ../src/libstate.a
	g++ $(CXXFLAGS) $(INCPATH) dbtest.cc $(LIBPATH) -lstate -o dbtest

# pngen

timers.o: 
	g++ $(CXXFLAGS) $(INCPATH) -c timers.cc -o timers.o

pngen.o: 
	g++ $(CXXFLAGS) $(INCPATH) -c pngen.cc -o pngen.o

pngen: pngen.o timers.o ../src/libstate.a
	g++ $(CXXFLAGS) pngen.o timers.o $(LIBPATH) -lstate -lpnfront -o pngen

# statediff

statediff.o: statediff.cc
	g++ $(CXXFLAGS) $(INCPATH) -c statediff.cc -o statediff.o

statediff: statediff.o ../src/libstate.a
	g++ $(CXXFLAGS) statediff.o $(LIBPATH) -lstate -o statediff

# testcoll

testcoll.o: testcoll.cc
	g++ $(CXXFLAGS) $(INCPATH) -c testcoll.cc -o testcoll.o

testcoll: testcoll.o ../src/libstate.a
	g++ $(CXXFLAGS) testcoll.o $(LIBPATH) -lstate -o testcoll

