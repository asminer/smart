
# $Id$

all: revno libstate.a

clean:
	touch statelib.o libstate.a
	rm *.o libstate.a

install: libstate.a
	cp libstate.a ../lib
	cp statelib.h ../include
	cp libstate.a ../../lib
	cp statelib.h ../../include

revno:
	./BuildRevNo

revision.h:
	./BuildRevNo

LIBS=coll.o statelib.o bst_db.o splaydb.o rb_db.o hash_db.o

libstate.a: $(LIBS)
	ar -r libstate.a $(LIBS)
	ranlib libstate.a

# dependencies

statelib.o: statelib.h statelib.cc revision.h coll.h 
statelib.o: splaydb.h rb_db.h hash_db.h

coll.o: coll.h coll.cc

bst_db.o: coll.h stack.h bst_db.h bst_db.cc

splaydb.o: coll.h bst_db.h splaydb.h splaydb.cc

rb_db.o: coll.h bst_db.h rb_db.h rb_db.cc

hash_db.o: coll.h hash_db.h hash_db.cc
