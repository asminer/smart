
# $Id$

#
#  Makefile for Main module: common section
#

SOURCES = \
	compile.cc tables.cc fnlib.cc api.cc

GENOBJS = lex.yy.o smart.tab.o

ALLOBJS = $(GENOBJS) $(SOURCES:.cc=.o)

CPPFLAGS 	= $(PLAT_FLAGS) -Wall 

timestamp: $(ALLOBJS)
	touch timestamp

makefile: $(SOURCEDIR)/make.mod $(SOURCEDIR)/../make.common
	cd $(SOURCEDIR); ./BuildMake

depend:
	touch $(SOURCEDIR)/smart.tab.h
	cd $(SOURCEDIR); $(CC) $(CPPFLAGS) -MM $(SOURCES) > $(OBJECTDIR)/make.depend
	rm $(SOURCEDIR)/smart.tab.h

clean :
	rm -f *.o lex.yy.cc smart.tab.c smart.tab.h $(SOURCEDIR)/smart.tab.h

lex.yy.cc : smart.l smart.tab.h
	flex $(SOURCEDIR)/smart.l

smart.tab.h smart.tab.c : smart.y 
	bison -d $(SOURCEDIR)/smart.y -o smart.tab.c
	rm -f $(SOURCEDIR)/smart.tab.h
	ln -s $(OBJECTDIR)/smart.tab.h $(SOURCEDIR)

