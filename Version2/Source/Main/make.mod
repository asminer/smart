
# $Id$

#
#  Makefile for Main module: common section
#

SOURCES = foo.cc

ALLOBJS = $(SOURCES:.cc=.o) 

CPPFLAGS 	= $(PLAT_FLAGS) -Wall 

all : $(ALLOBJS)

makefile: $(SOURCEDIR)/make.mod $(SOURCEDIR)/../make.common
	cd $(SOURCEDIR); ./BuildMake

depend:
	cd $(SOURCEDIR); $(CC) $(CPPFLAGS) -MM $(SOURCES) > $(OBJECTDIR)/make.depend

clean :
	rm -f *.o


