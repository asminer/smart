
# $Id$

#
#  Makefile for Language module: common section
#

SOURCES = \
	streams.cc types.cc options.cc timers.cc \
	errors.cc memtrack.cc api.cc docs.cc

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

