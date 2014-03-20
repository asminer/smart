
# $Id$

lib_LTLIBRARIES = librng.la

_revision:
	./BuildRevNo

revison.h:
	./BuildRevNo

BUILT_SOURCES = _revision revision.h
CLEANFILES = revision.h

librng_la_SOURCES = \
	rng.h \
  rng.cc 

include_HEADERS = rng.h

EXTRA_DIST = BuildRevNo revision.h
