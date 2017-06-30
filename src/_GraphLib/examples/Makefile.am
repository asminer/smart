
noinst_PROGRAMS = \
  grtest triples renumber scctest

check_PROGRAMS = \
  renumber scctest triples

TESTS = \
  renumber scctest triples

AM_CPPFLAGS = -I$(srcdir)/../ -I$(srcdir)/../../_IntSets -I$(srcdir)/../../_RngLib -I$(srcdir)/../../_GraphLib

grtest_SOURCES = grtest.cc ../graphlib.cc ../sccs.cc ../../_IntSets/intset.cc
# grtest_LDADD =  ../../_IntSets/libintset.la

triples_SOURCES = triples.cc ../graphlib.cc ../sccs.cc ../../_IntSets/intset.cc
triples_LDADD = ../../_RngLib/librng.la

renumber_SOURCES = renumber.cc ../graphlib.cc ../sccs.cc ../../_IntSets/intset.cc

scctest_SOURCES = scctest.cc ../graphlib.cc ../sccs.cc ../../_IntSets/intset.cc

