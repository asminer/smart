
# $Id$

#
#  Makefile for Main module: common section
#

SOURCES = foo.cc

GENOBJS = lex.yy.o

ALLOBJS = $(SOURCES:.cc=.o) $(GENOBJS)

CPPFLAGS 	= $(PLAT_FLAGS) -Wall 

all : $(ALLOBJS)

makefile: $(SOURCEDIR)/make.mod $(SOURCEDIR)/../make.common
	cd $(SOURCEDIR); ./BuildMake

depend:
	cd $(SOURCEDIR); $(CC) $(CPPFLAGS) -MM $(SOURCES) > $(OBJECTDIR)/make.depend

clean :
	rm -f *.o lex.yy.cc

lex.yy.cc : smart.l smart.tab.h
	flex $(SOURCEDIR)/smart.l

