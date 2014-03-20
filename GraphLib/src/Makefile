
# $Id$

INCPATH=-I../../include

OBJS= graphlib.o sccs.o 

all: revno libgraph.a

install: revno libgraph.a
	cp libgraph.a ../lib
	cp graphlib.h ../include
	cp libgraph.a ../../lib
	cp graphlib.h ../../include

clean:
	touch $(OBJS) libgraph.a; rm $(OBJS) libgraph.a

revno:
	./BuildRevNo

revision.h:
	./BuildRevNo

%.o: %.cc
	g++ $(CXXFLAGS) $(INCPATH) -c $< -o $@

libgraph.a: $(OBJS)
	ar -r libgraph.a $(OBJS)
	ranlib libgraph.a

# Dependencies

graphlib.o: graphlib.h graphlib.cc revision.h sccs.h

sccs.o: graphlib.h sccs.cc sccs.h

