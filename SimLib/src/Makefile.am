
# $Id$

lib_LTLIBRARIES = libsim.la libsimmdl.la

_revision:
	./BuildRevNo

revison.h:
	./BuildRevNo

BUILT_SOURCES = _revision revision.h
CLEANFILES = revision.h

libsim_la_SOURCES = \
	sim.h \
  sim.cc \
  normal.h \
  normal.cc \
  autocorrelation.h \
  autocorrelation.cc

libsimmdl_la_SOURCES = \
  sim-models.h \
  sim-models.cc \
  normal.h \
  normal.cc \
  autocorrelation.h \
  autocorrelation.cc


include_HEADERS = sim.h sim-models.h

EXTRA_DIST = BuildRevNo revision.h
