
# $Id$

noinst_PROGRAMS = \
  grtest triples

AM_CPPFLAGS = -I../src -I../../include

grtest_SOURCES = grtest.cc
grtest_LDADD = ../src/libgraph.la ../../lib/libintset.la

triples_SOURCES = triples.cc
triples_LDADD = ../src/libgraph.la ../../lib/libintset.la ../../lib/librng.la
