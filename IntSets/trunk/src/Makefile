
# $Id$

OBJS= intset.o vectlib.o
LIBS= libsv.a libintset.a
HEADERS= intset.h vectlib.h

all: revno $(LIBS)

clean:
	touch $(OBJS); rm $(OBJS)
	touch $(LIBS); rm $(LIBS)

install: $(LIBS)
	cp $(LIBS) ../lib
	cp $(HEADERS) ../include
	cp $(LIBS) ../../../lib
	cp $(HEADERS) ../../../include



revno revision.h:
	./BuildRevNo

intset.o: intset.h intset.cc revision.h
	g++ $(CXXFLAGS) -c intset.cc 

vectlib.o: vectlib.h vectlib.cc revision.h
	g++ $(CXXFLAGS) -c vectlib.cc 

libsv.a: vectlib.o
	ar -r libsv.a vectlib.o
	ranlib libsv.a

libintset.a: intset.o
	ar -r libintset.a intset.o
	ranlib libintset.a

