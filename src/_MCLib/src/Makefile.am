
# $Id$

lib_LTLIBRARIES = libmc.la

_revision:
	./BuildRevNo

revison.h:
	./BuildRevNo

BUILT_SOURCES = _revision revision.h
CLEANFILES = revision.h

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
  -I../../GraphLib/src \
  -I../../LSLib/src \
  -I../../IntSets/src \
  -I../../RngLib/src

include_HEADERS = mclib.h

EXTRA_DIST = BuildRevNo revision.h
