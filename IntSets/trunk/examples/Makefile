
# $Id$

# all: sieve sbv_test siv_test scanner hist
all: istest sieve

GENFILES= sieve sbv_test siv_test scanner hist sieve.o sbv_test.o siv_test.o scanner.o timers.o hist.o istest istest.o

clean:
	touch $(GENFILES); rm $(GENFILES)

check:	istest
	./istest *.ist



sieve: sieve.o
	g++ $(CXXFLAGS) -L../src/ sieve.o -lintset -o sieve

sieve.o: sieve.cc ../src/intset.h
	g++ $(CXXFLAGS) -I../src/ -c sieve.cc

sbv_test: sbv_test.o ../src/libsv.a
	g++ $(CXXFLAGS) sbv_test.o ../src/ -lsv -o sbv_test

sbv_test.o: sbv_test.cc
	g++ $(CXXFLAGS) -I../src/ -c sbv_test.cc

siv_test: siv_test.o ../src/libsv.a
	g++ $(CXXFLAGS) siv_test.o -L../src/ -lsv -o siv_test

siv_test.o: siv_test.cc
	g++ $(CXXFLAGS) -I../src/ -c siv_test.cc

scanner: scanner.o ../src/libsv.a timers.o
	g++ $(CXXFLAGS) scanner.o timers.o -L../src/ -lsv -o scanner

scanner.o: scanner.cc
	g++ $(CXXFLAGS) -I../src/ -c scanner.cc

hist: hist.o ../src/libsv.a timers.o
	g++ $(CXXFLAGS) hist.o timers.o -L../src/ -lsv -o hist

hist.o: hist.cc
	g++ $(CXXFLAGS) -I../src/ -c hist.cc

timers.o: timers.h timers.cc
	g++ $(CXXFLAGS) -c timers.cc

istest: istest.o ../src/intset.o
	g++ istest.o ../src/intset.o -o istest

istest.o: istest.cc
	g++ $(CXXFLAGS) -c istest.cc


