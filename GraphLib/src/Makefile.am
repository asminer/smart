
# $Id$

lib_LTLIBRARIES = libgraph.la

_revision:
	./BuildRevNo

revison.h:
	./BuildRevNo

BUILT_SOURCES = _revision revision.h
CLEANFILES = revision.h

libgraph_la_SOURCES = \
	graphlib.h \
  graphlib.cc \
  sccs.h \
  sccs.cc

AM_CPPFLAGS = -I../src -I../../IntSets/src

include_HEADERS = graphlib.h

EXTRA_DIST = BuildRevNo revision.h
