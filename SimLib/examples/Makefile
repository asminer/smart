
# $Id$

EXES=dice oz_test bssq_test sms_test
BINS=tester.o oz_test.o bssq_test.o sms_test.o

all: $(EXES)

INCPATH=-I../src -I../../RngLib/include
LIBPATH=-L../src -L../../RngLib/lib

dice: dice.cc ../src/libsim.a
	g++ $(CXXFLAGS) $(INCPATH) dice.cc $(LIBPATH) -lsim -o dice

tester.o: tester.cc tester.h
	g++ -c $(CXXFLAGS) $(INCPATH) tester.cc

oz_test.o: oz_test.cc tester.h
	g++ -c $(CXXFLAGS) $(INCPATH) oz_test.cc

oz_test: oz_test.o tester.o ../src/libsimmdl.a
	g++ $(LIBPATH) tester.o oz_test.o -lrng -lsimmdl -o oz_test

bssq_test.o: bssq_test.cc tester.h
	g++ -c $(CXXFLAGS) $(INCPATH) bssq_test.cc

bssq_test: bssq_test.o tester.o ../src/libsimmdl.a
	g++ $(LIBPATH) tester.o bssq_test.o -lrng -lsimmdl -o bssq_test

sms_test.o: sms_test.cc tester.h
	g++ -c $(CXXFLAGS) $(INCPATH) sms_test.cc

sms_test: sms_test.o tester.o ../src/libsimmdl.a
	g++ $(LIBPATH) tester.o sms_test.o -lrng -lsimmdl -o sms_test

clean:
	touch $(EXES) $(BINS); rm $(EXES) $(BINS)
