
# $Id$

#
#  Makefile for Chains module: common section
#

SOURCES = \
	procs.cc

ALLOBJS = $(SOURCES:.cc=.o) 

CPPFLAGS 	= $(PLAT_FLAGS) -Wall 

timestamp: $(ALLOBJS)
	touch timestamp

makefile: $(SOURCEDIR)/make.mod $(SOURCEDIR)/../make.common
	cd $(SOURCEDIR); ./BuildMake

depend:
	touch $(SOURCEDIR)/../Main/smart.tab.h
	cd $(SOURCEDIR); $(CC) $(CPPFLAGS) -MM $(SOURCES) > $(OBJECTDIR)/make.depend
	rm $(SOURCEDIR)/../Main/smart.tab.h

clean :
	rm -f *.o

