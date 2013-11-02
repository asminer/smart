
# $Id$

all: revno libsim.a libsimmdl.a

clean:
	touch sim.o normal.o autocorrelation.o
	rm sim.o normal.o autocorrelation.o
	touch libsim.a; rm libsim.a

install: libsim.a
	cp libsim.a ../lib
	cp sim.h ../include
	cp libsim.a ../../../lib
	cp sim.h ../../../include

revno:
	./BuildRevNo

revision.h:
	./BuildRevNo

autocorrelation.o: autocorrelation.h autocorrelation.cc
	g++ $(CXXFLAGS) -c autocorrelation.cc

normal.o: normal.h normal.cc
	g++ $(CXXFLAGS) -c normal.cc

sim.o: sim.h sim.cc revision.h normal.h
	g++ $(CXXFLAGS) -c sim.cc

sim-models.o: sim-models.h sim-models.cc normal.h autocorrelation.h
	g++ $(CXXFLAGS) -c sim-models.cc

libsim.a: sim.o normal.o autocorrelation.o
	ar -r libsim.a sim.o normal.o normal.o autocorrelation.o
	ranlib libsim.a

libsimmdl.a: sim-models.o normal.o autocorrelation.o
	ar -r libsimmdl.a sim-models.o normal.o autocorrelation.o
	ranlib libsimmdl.a

