
#
#  Makefile
#
#  Because some of these sources are NOT used for smart.
#


CC = g++
CXX = g++
CPPFLAGS = -ggdb -DDEVELOPMENT_CODE
#CPPFLAGS = -O3

EXECS = testflat testmdd testmm

all: $(EXECS)

testflat: testflat.o streams.o stateheap.o flatss.o errors.o 
	$(CXX) $(CPPFLAGS) -o testflat testflat.o streams.o stateheap.o flatss.o errors.o

testflat.o: testflat.cc flatss.h

streams.o: ../Base/streams.cc ../Base/streams.h
	$(CXX) $(CPPFLAGS) -c -o streams.o ../Base/streams.cc

errors.o: ../Base/errors.cc ../Base/errors.h
	$(CXX) $(CPPFLAGS) -c -o errors.o ../Base/errors.cc

results.o: ../Language/results.cc ../Language/results.h
	$(CXX) $(CPPFLAGS) -c -o results.o ../Language/results.cc

intset.o: ../Templates/intset.cc ../Templates/intset.h
	$(CXX) $(CPPFLAGS) -c -o intset.o ../Templates/intset.cc

stateheap.o: stateheap.h stateheap.cc
	$(CXX) $(CPPFLAGS) -DEXTRACTED_FROM_SMART -c stateheap.cc

flatss.o: flatss.h flatss.cc stateheap.h
	$(CXX) $(CPPFLAGS) -DDEBUG_COMPACT -c flatss.cc

testmdd: testmdd.o mdds.o mdd_ops.o streams.o errors.o intset.o
	$(CXX) $(CPPFLAGS) -o testmdd testmdd.o mdds.o mdd_ops.o streams.o errors.o intset.o

testmdd.o: testmdd.cc mdds.h mdd_ops.h
mdds.o: mdds.h mdds.cc
mdd_ops.o: mdds.h mdd_ops.h mdd_ops.cc

testmm: testmm.o mdds_malloc.o mddops_malloc.o streams.o errors.o intset.o
	$(CXX) $(CPPFLAGS) -o testmm testmm.o mdds_malloc.o mddops_malloc.o streams.o errors.o intset.o

testmm.o: testmm.cc mdds_malloc.h mddops_malloc.h
mdds_malloc.o: mdds_malloc.h mdds_malloc.cc
mddops_malloc.o: mdds_malloc.h mddops_malloc.h mddops_malloc.cc

clean:
	rm *.o $(EXECS)
