
SUBDIRS = . examples

lib_LTLIBRARIES = libmc.la

libmc_la_SOURCES = \
  error.cc \
  markov_chain.cc \
  vanishing.cc 

AM_CPPFLAGS = \
  -I$(srcdir)/../_GraphLib \
  -I$(srcdir)/../_LSLib \
  -I$(srcdir)/../_IntSets \
  -I$(srcdir)/../_RngLib

