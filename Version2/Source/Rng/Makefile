
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

EXECS = square multiply makeB mtcheck checkmult dist makeJump jumpcheck

all: $(EXECS)

square: square.o multstrm.o streams.o timers.o
	g++ $(CPPFLAGS) -o square square.o multstrm.o streams.o timers.o

multiply: multiply.o multstrm.o streams.o timers.o
	g++ $(CPPFLAGS) -o multiply multiply.o multstrm.o streams.o timers.o

checkmult: checkmult.o multstrm.o streams.o timers.o
	g++ $(CPPFLAGS) -o checkmult checkmult.o multstrm.o streams.o timers.o

makeB: makeB.o multstrm.o streams.o timers.o
	g++ $(CPPFLAGS) -o makeB makeB.o multstrm.o streams.o timers.o

mtcheck: mtcheck.o multstrm.o streams.o mtwist.o
	g++ $(CPPFLAGS) -o mtcheck mtcheck.o multstrm.o streams.o mtwist.o

dist: dist.o mtwist.o
	g++ $(CPPFLAGS) -o dist dist.o mtwist.o

makeJump: makeJump.o multstrm.o streams.o
	g++ $(CPPFLAGS) -o makeJump makeJump.o multstrm.o streams.o

jumpcheck: jumpcheck.o streams.o rng.o mtwist.o
	g++ $(CPPFLAGS) -o jumpcheck jumpcheck.o streams.o rng.o mtwist.o

timers.o: ../Base/timers.cc ../Base/timers.h
	g++ $(CPPFLAGS) -c -o timers.o ../Base/timers.cc

streams.o: ../Base/streams.cc ../Base/streams.h
	g++ $(CPPFLAGS) -c -o streams.o ../Base/streams.cc

multstrm.o: multstrm.h multstrm.cc

rng.o: rng.h jump.h rng.cc ../Base/streams.h


clean:
	rm *.o $(EXECS)
