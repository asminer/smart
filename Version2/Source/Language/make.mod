
# $Id$

#
#  Makefile for Language module: common section
#

SOURCES = \
	results.cc	exprs.cc	infinity.cc	\
	sets.cc 	stmts.cc 	\
	baseops.cc	operators.cc	casting.cc \
	variables.cc	functions.cc	arrays.cc \
	converge.cc	\
	measures.cc	\
	strings.cc	\
	api.cc

ALLOBJS = $(SOURCES:.cc=.o) 

CPPFLAGS 	= $(PLAT_FLAGS) -Wall 

timestamp: $(ALLOBJS)
	touch timestamp

makefile: $(SOURCEDIR)/make.mod $(SOURCEDIR)/../make.common
	cd $(SOURCEDIR); ./BuildMake

depend:
	touch $(SOURCEDIR)/../Main/smart.tab.h
	cd $(SOURCEDIR); $(CXX) $(CPPFLAGS) -MM $(SOURCES) > $(OBJECTDIR)/make.depend
	rm -f $(SOURCEDIR)/../Main/smart.tab.h $(OBJECTDIR)/../Main/smart.tab.h

clean :
	rm -f *.o

