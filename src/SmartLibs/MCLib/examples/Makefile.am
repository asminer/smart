
# $Id$

noinst_PROGRAMS = \
  birthdeath \
  walk \
  mcsolver \
  oz \
  univ

AM_CPPFLAGS = -I../src -I../../Timer/src -I../../LSLib/src -I../../GraphLib/src -I../../MCParse/src

birthdeath_SOURCES = birthdeath.cc 
birthdeath_LDADD = ../src/libmc.la ../../LSLib/src/libls.la ../../GraphLib/src/libgraph.la ../../IntSets/src/libintset.la ../../Timer/src/libtimer.la

walk_SOURCES = walk.cc 
walk_LDADD = ../src/libmc.la ../../LSLib/src/libls.la ../../GraphLib/src/libgraph.la ../../IntSets/src/libintset.la ../../Timer/src/libtimer.la

mcsolver_SOURCES = mcsolver.cc 
mcsolver_LDADD = ../src/libmc.la ../../MCParse/src/libmcparse.la ../../LSLib/src/libls.la ../../GraphLib/src/libgraph.la ../../IntSets/src/libintset.la ../../Timer/src/libtimer.la

oz_SOURCES = oz.cc
oz_LDADD = ../src/libmc.la ../../LSLib/src/libls.la ../../GraphLib/src/libgraph.la ../../IntSets/src/libintset.la ../../Timer/src/libtimer.la

univ_SOURCES = univ.cc
univ_LDADD = ../src/libmc.la ../../LSLib/src/libls.la ../../GraphLib/src/libgraph.la ../../IntSets/src/libintset.la ../../Timer/src/libtimer.la

TESTS = \
  scc0.dtmc \
  scc1.dtmc \
  scc2.dtmc \
  scc3.dtmc

LOG_COMPILER = ./mcsolver
AM_LOG_FLAGS = -q
