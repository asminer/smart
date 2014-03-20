
# $Id$


INCPATH=-I../../include

OBJS= mclib.o mcbase.o mc_irred.o mc_absorb.o mc_general.o \
	vanish.o hyper.o poiss.o 


all: revno libmc.a

install: libmc.a
	cp libmc.a ../lib
	cp mclib.h ../include
	cp libmc.a ../../lib
	cp mclib.h ../../include

clean:
	touch $(OBJS) libmc.a; rm $(OBJS) libmc.a

revno:
	./BuildRevNo

revision.h:
	./BuildRevNo


%.o: %.cc
	g++ $(CXXFLAGS) $(INCPATH) -c $< -o $@

libmc.a: $(OBJS)
	ar -r libmc.a $(OBJS)
	ranlib libmc.a

# Dependencies

mclib.o: \
	mclib.h mclib.cc revision.h

mcbase.o: \
	mclib.h hyper.h mcbase.h mcbase.cc 

mc_irred.o: \
	mclib.h mcbase.h mc_irred.h mc_irred.cc

mc_absorb.o: \
	mclib.h mcbase.h hyper.h mc_absorb.h mc_absorb.cc

mc_general.o: \
	mclib.h hyper.h mcbase.h mc_general.h mc_general.cc 

vanish.o: \
	mclib.h hyper.h mc_absorb.h mc_general.h vanish.cc

hyper.o: circular.h hyper.h hyper.cc
poiss.o: poiss.cc

