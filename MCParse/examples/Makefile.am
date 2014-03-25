
# $Id$

noinst_PROGRAMS = \
  parsetest \
  renumber

AM_CPPFLAGS = -I../src -I../../GraphLib/src

parsetest_SOURCES = parsetest.cc
parsetest_LDADD = ../src/libmcparse.la ../../GraphLib/src/libgraph.la ../../IntSets/src/libintset.la

renumber_SOURCES = renumber.cc
renumber_LDADD = ../src/libmcparse.la ../../GraphLib/src/libgraph.la ../../IntSets/src/libintset.la

