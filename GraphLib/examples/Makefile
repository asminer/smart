
# $Id$

INCPATH=-I../src -I../../include
LIBPATH=-L../src -L../../lib

EXES = grtest
OBJS =

all: $(EXES)

clean:
	touch $(EXES) $(OBJS); rm $(EXES) $(OBJS)

# grtest

grtest: grtest.cc ../src/libgraph.a 
	g++ $(CXXFLAGS) $(INCPATH) grtest.cc $(LIBPATH) -lgraph -lintset -o grtest 

