
# $Id$

#
#  Makefile for Main module: common section
#

SOURCES = foo.cc

GENOBJS = lex.yy.o smart.tab.o

ALLOBJS = $(GENOBJS) $(SOURCES:.cc=.o)

CPPFLAGS 	= $(PLAT_FLAGS) -Wall 

all : smart.tab.h $(ALLOBJS)

makefile: $(SOURCEDIR)/make.mod $(SOURCEDIR)/../make.common
	cd $(SOURCEDIR); ./BuildMake

depend:
	cd $(SOURCEDIR); $(CC) $(CPPFLAGS) -MM $(SOURCES) > $(OBJECTDIR)/make.depend

clean :
	rm -f *.o lex.yy.cc smart.tab.c smart.tab.h

lex.yy.cc : smart.l smart.tab.h
	flex $(SOURCEDIR)/smart.l

smart.tab.h smart.tab.c : smart.y
	bison -d $(SOURCEDIR)/smart.y -o smart.tab.c
	mv smart.tab.h ../../Source/Language
	ln -s -f ../../Source/Language/smart.tab.h .


