
#
#  Makefile
#
#  Because some of these sources are NOT used for smart.
#
#  I.e., we have sources here to do one-time computations for RNG stuff.


CC = g++
CXX = g++
# CPPFLAGS = -g 
CPPFLAGS = -O3

square: square.o multstrm.o streams.o timers.o
	g++ $(CPPFLAGS) -o square square.o multstrm.o streams.o timers.o

multiply: multiply.o multstrm.o streams.o timers.o
	g++ $(CPPFLAGS) -o multiply multiply.o multstrm.o streams.o timers.o

makeB: makeB.o multstrm.o streams.o timers.o
	g++ $(CPPFLAGS) -o makeB makeB.o multstrm.o streams.o timers.o

mtcheck: mtcheck.o multstrm.o streams.o mtwist.o
	g++ $(CPPFLAGS) -o mtcheck mtcheck.o multstrm.o streams.o mtwist.o

timers.o: ../Base/timers.cc ../Base/timers.h
	g++ $(CPPFLAGS) -c -o streams.o ../Base/streams.cc


streams.o: ../Base/streams.cc ../Base/streams.h
	g++ $(CPPFLAGS) -c -o streams.o ../Base/streams.cc

multstrm.o: multstrm.h multstrm.cc


clean:
	rm *.o square multiply makeB mtcheck
