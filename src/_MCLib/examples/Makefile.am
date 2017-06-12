
noinst_PROGRAMS = \
  birthdeath \
  mcsolver \
  period \
  oz \
  tta_test \
  univ \
  walk 

check_PROGRAMS = \
  period \
  tta_test

TESTS = \
  period \
  tta_test


MY_SOURCES = \
  ../../_LSLib/lslib.cc \
  ../../_GraphLib/graphlib.cc ../../_GraphLib/sccs.cc \
  ../../_IntSets/intset.cc ../../_Timer/timerlib.cc 

MC_SOURCES = \
  ../markov_chain.cc

AM_CPPFLAGS = -I$(srcdir)/../ -I$(srcdir)/../../_Timer -I$(srcdir)/../../_LSLib -I$(srcdir)/../../_GraphLib 

birthdeath_SOURCES = birthdeath.cc $(MY_SOURCES)
birthdeath_LDADD = ../libmc.la 

walk_SOURCES = walk.cc $(MY_SOURCES)
walk_LDADD = ../libmc.la 

mcsolver_SOURCES = mcsolver.cc mcparse.cc $(MY_SOURCES)
mcsolver_LDADD = ../libmc.la 

oz_SOURCES = oz.cc $(MY_SOURCES)
oz_LDADD = ../libmc.la 

univ_SOURCES = univ.cc $(MY_SOURCES)
univ_LDADD = ../libmc.la 

period_SOURCES = period.cc $(MY_SOURCES)
period_LDADD = ../libmc.la

tta_test_SOURCES = tta_test.cc $(MY_SOURCES)
tta_test_LDADD = ../libmc.la

# 
# TESTS = \
  # scc0.dtmc \
  # scc1.dtmc \
  # scc2.dtmc \
  # scc3.dtmc
# 
# LOG_COMPILER = ./mcsolver
# AM_LOG_FLAGS = -q
