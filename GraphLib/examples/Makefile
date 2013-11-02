
# $Id$

INCPATH=-I../src -I../../IntSets/include
LIBPATH=-L../src -L../../IntSets/lib

EXES = grtest
OBJS =

all: $(EXES)

clean:
	touch $(EXES) $(OBJS); rm $(EXES) $(OBJS)

# grtest

grtest: grtest.cc ../src/libgraph.a 
	g++ $(CXXFLAGS) $(INCPATH) grtest.cc $(LIBPATH) -lgraph -lintset -o grtest 

