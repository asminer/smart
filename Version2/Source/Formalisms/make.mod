
# $Id$

#
#  Makefile for Formalisms module: common section
#

SOURCES = \
	api.cc

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

