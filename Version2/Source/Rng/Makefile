
#
#  Makefile
#
#  Because some of these sources are NOT used for smart.
#
#  I.e., we have sources here to do one-time computations for RNG stuff.


CC = g++
CXX = g++
#CPPFLAGS = -g 
CPPFLAGS = -O3

EXECS = square multiply makeB mtcheck mtcheck2 dist makeJump jumpcheck generate

all: $(EXECS)

square: square.o multstrm.o streams.o timers.o
	g++ $(CPPFLAGS) -o square square.o multstrm.o streams.o timers.o

multiply: multiply.o multstrm.o streams.o timers.o
	g++ $(CPPFLAGS) -o multiply multiply.o multstrm.o streams.o timers.o

makeB: makeB.o multstrm.o streams.o timers.o errors.o
	g++ $(CPPFLAGS) -o makeB makeB.o multstrm.o streams.o timers.o errors.o

mtcheck: mtcheck.o multstrm.o streams.o mtwist.o
	g++ $(CPPFLAGS) -o mtcheck mtcheck.o multstrm.o streams.o mtwist.o

mtcheck2: mtcheck2.o multstrm.o streams.o mtwist.o rng.o
	g++ $(CPPFLAGS) -o mtcheck2 mtcheck2.o multstrm.o streams.o mtwist.o rng.o

dist: dist.o mtwist.o
	g++ $(CPPFLAGS) -o dist dist.o mtwist.o

makeJump: makeJump.o multstrm.o streams.o
	g++ $(CPPFLAGS) -o makeJump makeJump.o multstrm.o streams.o

jumpcheck: jumpcheck.o streams.o rng.o mtwist.o
	g++ $(CPPFLAGS) -o jumpcheck jumpcheck.o streams.o rng.o mtwist.o

generate: generate.o rng.o mtwist.o
	g++ $(CPPFLAGS) -o generate generate.o rng.o mtwist.o

timers.o: ../Base/timers.cc ../Base/timers.h
	g++ $(CPPFLAGS) -c -o timers.o ../Base/timers.cc

streams.o: ../Base/streams.cc ../Base/streams.h
	g++ $(CPPFLAGS) -c -o streams.o ../Base/streams.cc

errors.o: ../Base/errors.cc ../Base/errors.h
	g++ $(CPPFLAGS) -c -o errors.o ../Base/errors.cc

multstrm.o: multstrm.h multstrm.cc

rng.o: rng.h jump.h rng.cc ../Base/streams.h


clean:
	rm *.o $(EXECS)
