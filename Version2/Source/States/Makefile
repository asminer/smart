
#
#  Makefile
#
#  Because some of these sources are NOT used for smart.
#


CC = g++
CXX = g++
#CPPFLAGS = -g 
CPPFLAGS = -O3

EXECS = testflat

all: $(EXECS)

testflat: testflat.o streams.o 
	g++ $(CPPFLAGS) -o testflat testflat.o streams.o 

testflat.o: testflat.cc 

streams.o: ../Base/streams.cc ../Base/streams.h
	g++ $(CPPFLAGS) -c -o streams.o ../Base/streams.cc


clean:
	rm *.o $(EXECS)
