
# $Id$

#
#  Makefile for Language module: common section
#

SOURCES = \
	types.cc	exprs.cc	sets.cc 	stmts.cc \
	infinity.cc	\
	baseops.cc	operators.cc	casting.cc \
	variables.cc	functions.cc	arrays.cc \
	initfuncs.cc

ALLOBJS = $(SOURCES:.cc=.o) 

CPPFLAGS 	= $(PLAT_FLAGS) -Wall 

all : $(ALLOBJS)

makefile: $(SOURCEDIR)/make.mod $(SOURCEDIR)/../make.common
	cd $(SOURCEDIR); ./BuildMake

depend:
	cd $(SOURCEDIR); touch smart.tab.h; $(CC) $(CPPFLAGS) -MM $(SOURCES) > $(OBJECTDIR)/make.depend

clean :
	rm -f *.o smart.tab.h


