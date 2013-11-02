
# $Id$

all: axzero axb

GENFILES= axzero axb timers.o matrix.o axzero.o axb.o 

clean:
	touch $(GENFILES); rm $(GENFILES)


axzero: axzero.o timers.o matrix.o ../src/libls.a
	g++ $(CXXFLAGS) axzero.o timers.o matrix.o -L../src -lls -o axzero

axb: axb.o timers.o matrix.o ../src/libls.a
	g++ $(CXXFLAGS) axb.o timers.o matrix.o -L../src -lls -o axb

matrix.o: matrix.cc matrix.h
	g++ $(CXXFLAGS) -I../src -c matrix.cc
timers.o: timers.cc timers.h
axzero.o: axzero.cc timers.h matrix.h ../src/lslib.h
	g++ $(CXXFLAGS) -I../src -c axzero.cc
axb.o: axb.cc timers.h matrix.h ../src/lslib.h
	g++ $(CXXFLAGS) -I../src -c axb.cc

