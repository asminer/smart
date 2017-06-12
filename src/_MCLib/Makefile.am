
SUBDIRS = . examples

lib_LTLIBRARIES = libmc.la

libmc_la_SOURCES = \
  error.cc \
  markov_chain.cc \
  \
  mclib.cc \
  mcbase.cc \
  mc_irred.cc \
  mc_absorb.cc \
  mc_general.cc \
  vanish.cc \
  hyper.cc \
  poiss.cc

AM_CPPFLAGS = \
  -I$(srcdir)/../_GraphLib \
  -I$(srcdir)/../_LSLib \
  -I$(srcdir)/../_IntSets \
  -I$(srcdir)/../_RngLib

