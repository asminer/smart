
# $Id$

EXES= rawtest jumptest grid uniftest expotest autocorr shuffle

all: $(EXES)

clean:
	touch $(EXES); rm $(EXES)

#

rawtest: rawtest.cc
	g++ $(CXXFLAGS) -I../src rawtest.cc -L../src -lrng -o rawtest

autocorr: autocorr.cc
	g++ $(CXXFLAGS) -I../src autocorr.cc -L../src -lrng -o autocorr

jumptest: jumptest.cc
	g++ $(CXXFLAGS) -I../src jumptest.cc -L../src -lrng -o jumptest

grid: grid.cc
	g++ $(CXXFLAGS) -I../src grid.cc -L../src -lrng -o grid

uniftest: uniftest.cc
	g++ $(CXXFLAGS) -I../src uniftest.cc -L../src -lrng -o uniftest

expotest: expotest.cc
	g++ $(CXXFLAGS) -I../src expotest.cc -L../src -lrng -o expotest

shuffle: shuffle.cc
	g++ $(CXXFLAGS) -I../src/ shuffle.cc -L../src -lrng -o shuffle
