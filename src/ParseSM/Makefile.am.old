
## $Id$

## Makefile.am -- Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libsmparse.la

libsmparse_la_SOURCES = \
  smartlex.l  smartyacc.y \
  lexer.h     lexer.cc \
  compile.h   compile.cc \
  parse_sm.h  parse_sm.cc

BUILT_SOURCES = smartyacc.h

AM_YFLAGS = -d
