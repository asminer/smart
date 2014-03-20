
# $Id$

lib_LTLIBRARIES = libmcparse.la 

_revision:
	./BuildRevNo

revison.h:
	./BuildRevNo

BUILT_SOURCES = _revision revision.h
CLEANFILES = revision.h

libmcparse_la_SOURCES = \
  mcparse.h \
  mcparse.cc 

include_HEADERS = mcparse.h

EXTRA_DIST = BuildRevNo revision.h
