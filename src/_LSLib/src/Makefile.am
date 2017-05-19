
# $Id$

lib_LTLIBRARIES = libls.la

_revision:
	./BuildRevNo

revison.h:
	./BuildRevNo

BUILT_SOURCES = _revision revision.h
CLEANFILES = revision.h

libls_la_SOURCES = \
  lslib.h \
  lslib.cc 

include_HEADERS = lslib.h

EXTRA_DIST = BuildRevNo revision.h
