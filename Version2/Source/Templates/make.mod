
# $Id$

#
#  Makefile for Templates (not all are strictly templates ;^)
#

SOURCES = \
	splay.cc	graphs.cc	listarray.cc	intset.cc

ALLOBJS = $(SOURCES:.cc=.o) 

CPPFLAGS 	= $(PLAT_FLAGS) -Wall 

timestamp: $(ALLOBJS)
	touch timestamp

makefile: $(SOURCEDIR)/make.mod $(SOURCEDIR)/../make.common
	cd $(SOURCEDIR); ./BuildMake

depend:
	cd $(SOURCEDIR); $(CC) $(CPPFLAGS) -MM $(SOURCES) > $(OBJECTDIR)/make.depend
	

clean :
	rm -f *.o

