
#
#  $Id$
#  
#  Makefile
#
#  Because some of these sources are NOT used for smart.
#


CC = g++
CXX = g++
CPPFLAGS = -g 
#CPPFLAGS = -O3

EXECS = graphtest latest

all: $(EXECS)

latest: latest.o listarray.o streams.o errors.o
	$(CXX) $(CPPFLAGS) -o latest latest.o streams.o listarray.o errors.o

latest.o: latest.cc listarray.h
listarray.o: listarray.h listarray.cc

graphtest: graphtest.o graphs.o streams.o errors.o
	$(CXX) $(CPPFLAGS) -o graphtest graphtest.o streams.o graphs.o errors.o

graphtest.o: graphtest.cc graphs.h
graphs.o: graphs.h graphs.cc

streams.o: ../Base/streams.cc ../Base/streams.h
	$(CXX) $(CPPFLAGS) -c -o streams.o ../Base/streams.cc

errors.o: ../Base/errors.cc ../Base/errors.h
	$(CXX) $(CPPFLAGS) -c -o errors.o ../Base/errors.cc

clean:
	rm *.o $(EXECS)
