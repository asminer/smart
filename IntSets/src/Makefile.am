
# $Id$

lib_LTLIBRARIES = libintset.la

_revision:
	./BuildRevNo

revison.h:
	./BuildRevNo

BUILT_SOURCES = _revision revision.h
CLEANFILES = revision.h

libintset_la_SOURCES = \
  intset.h \
  intset.cc

include_HEADERS = intset.h

EXTRA_DIST = BuildRevNo revision.h
