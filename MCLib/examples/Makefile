
# $Id$

CXX = g++
CC  = g++

all: birthdeath walk mcsolver

INCPATH=-I../src -I../../LSLib/include -I../../GraphLib/include -I../../MCParse/include
LIBPATH=-L../src -L../../LSLib/lib -L../../IntSets/lib -L../../GraphLib/lib -L../../MCParse/lib

LIBS=$(LIBPATH) -lmc -lls -lgraph -lintset

GENFILES= birthdeath birthdeath.o walk walk.o timers.o mcsolver.o mcsolver

clean:
	touch $(GENFILES); rm $(GENFILES)

check: all
	./SolverTest scc?.dtmc

# generic rule for compiling
%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCPATH) -c $< -o $@

# dependencies

birthdeath: birthdeath.o timers.o ../src/libmc.a 
birthdeath.o: birthdeath.cc ../src/mclib.h
walk:	walk.o timers.o ../src/libmc.a
walk.o: walk.cc ../src/mclib.h
mcsolver: mcsolver.o timers.o ../src/libmc.a
mcsolver.o: mcsolver.cc
timers.o: timers.cc timers.h

# Linking and other rules

birthdeath: 
	$(CXX) $(CXXFLAGS) birthdeath.o timers.o $(LIBS) -o birthdeath

walk:
	$(CXX) $(CXXFLAGS) walk.o timers.o $(LIBS) -o walk

mcsolver:
	$(CXX) $(CXXFLAGS) mcsolver.o timers.o $(LIBS) -lmcparse -o mcsolver

