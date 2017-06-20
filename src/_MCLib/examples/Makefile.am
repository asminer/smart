
noinst_PROGRAMS = \
  birthdeath \
  mcsolver \
  oz \
  univ \
  walk \
  acc_test \
  dist_test \
  period \
  revtr_test \
  ss_test \
  trans_test \
  tta_test 

check_PROGRAMS = \
  acc_test \
  dist_test \
  period \
  revtr_test \
  ss_test \
  trans_test \
  tta_test

TESTS = \
  acc_test \
  dist_test \
  period \
  revtr_test \
  ss_test \
  trans_test \
  tta_test


MY_SOURCES = \
  ../../_LSLib/lslib.cc \
  ../../_GraphLib/graphlib.cc ../../_GraphLib/sccs.cc \
  ../../_IntSets/intset.cc ../../_Timer/timerlib.cc ../../_Distros/distros.cc

# MC_SOURCES = \
#  ../markov_chain.cc

#
# All programs need this
#
LDADD = ../libmc.la

AM_CPPFLAGS = -I$(srcdir)/../ -I$(srcdir)/../../_Timer -I$(srcdir)/../../_LSLib -I$(srcdir)/../../_GraphLib 

#
# Program specific stuff
#

birthdeath_SOURCES = birthdeath.cc $(MY_SOURCES)

walk_SOURCES = walk.cc $(MY_SOURCES)

mcsolver_SOURCES = mcsolver.cc mcparse.cc $(MY_SOURCES)

oz_SOURCES = oz.cc $(MY_SOURCES)

univ_SOURCES = univ.cc $(MY_SOURCES)

period_SOURCES = period.cc $(MY_SOURCES)

tta_test_SOURCES = tta_test.cc mcbuilder.cc $(MY_SOURCES)

ss_test_SOURCES = ss_test.cc mcbuilder.cc $(MY_SOURCES)

trans_test_SOURCES = trans_test.cc mcbuilder.cc $(MY_SOURCES)

revtr_test_SOURCES = revtr_test.cc mcbuilder.cc $(MY_SOURCES)

acc_test_SOURCES = acc_test.cc mcbuilder.cc $(MY_SOURCES)

dist_test_SOURCES = dist_test.cc mcbuilder.cc $(MY_SOURCES)
