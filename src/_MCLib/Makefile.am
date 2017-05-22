
SUBDIRS = . examples

lib_LTLIBRARIES = libmc.la

libmc_la_SOURCES = \
  mclib.h \
  mclib.cc \
  mcbase.h \
  mcbase.cc \
  mc_irred.h \
  mc_irred.cc \
  mc_absorb.h \
  mc_absorb.cc \
  mc_general.h \
  mc_general.cc \
  vanish.h \
  vanish.cc \
  circular.h \
  hyper.h \
  hyper.cc \
  poiss.cc

AM_CPPFLAGS = \
  -I$(srcdir)/../_GraphLib \
  -I$(srcdir)/../_LSLib \
  -I$(srcdir)/../_IntSets \
  -I$(srcdir)/../_RngLib

