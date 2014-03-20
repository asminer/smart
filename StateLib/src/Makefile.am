
# $Id$

lib_LTLIBRARIES = libstate.la

_revision:
	./BuildRevNo

revison.h:
	./BuildRevNo

BUILT_SOURCES = _revision revision.h
CLEANFILES = revision.h

libstate_la_SOURCES = \
	statelib.h \
  statelib.cc \
  coll.h \
  coll.cc \
  stack.h \
  bst_db.h \
  bst_db.cc \
  splaydb.h \
  splaydb.cc \
  rb_db.h \
  rb_db.cc \
  hash_db.h \
  hash_db.cc

include_HEADERS = statelib.h

EXTRA_DIST = BuildRevNo revision.h
