
# $Id$

# mega makefile

SOURCEDIR = ../Source

vpath %.h $(SOURCEDIR)
vpath %.cc $(SOURCEDIR)

smart: language main
	g++ $(SOURCEDIR)/main.cc Language/*.o Main/*.o -o smart

language:
	cd Language; make

main:
	cd Main; make

clean:
	cd Language; make clean
	cd Main; make clean
