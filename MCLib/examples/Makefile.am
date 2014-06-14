
# $Id$

noinst_PROGRAMS = \
  birthdeath \
  walk \
  mcsolver \
  oz \
  univ

AM_CPPFLAGS = -I../src -I../../LSLib/src -I../../GraphLib/src -I../../MCParse/src

birthdeath_SOURCES = birthdeath.cc timers.h timers.cc
birthdeath_LDADD = ../src/libmc.la ../../LSLib/src/libls.la ../../GraphLib/src/libgraph.la ../../IntSets/src/libintset.la

walk_SOURCES = walk.cc timers.h timers.cc
walk_LDADD = ../src/libmc.la ../../LSLib/src/libls.la ../../GraphLib/src/libgraph.la ../../IntSets/src/libintset.la

mcsolver_SOURCES = mcsolver.cc timers.h timers.cc
mcsolver_LDADD = ../src/libmc.la ../../MCParse/src/libmcparse.la ../../LSLib/src/libls.la ../../GraphLib/src/libgraph.la ../../IntSets/src/libintset.la

oz_SOURCES = oz.cc
oz_LDADD = ../src/libmc.la ../../LSLib/src/libls.la ../../GraphLib/src/libgraph.la ../../IntSets/src/libintset.la

univ_SOURCES = univ.cc
univ_LDADD = ../src/libmc.la ../../LSLib/src/libls.la ../../GraphLib/src/libgraph.la ../../IntSets/src/libintset.la

TESTS = \
  scc0.dtmc \
  scc1.dtmc \
  scc2.dtmc \
  scc3.dtmc

LOG_COMPILER = ./mcsolver
AM_LOG_FLAGS = -q
