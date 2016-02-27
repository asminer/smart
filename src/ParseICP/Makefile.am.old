
## $Id$

## Makefile.am -- Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libicpparse.la

libicpparse_la_SOURCES = \
  icplex.l    icpyacc.y \
  lexer.h     lexer.cc \
  compile.h   compile.cc \
  parse_icp.h  parse_icp.cc

BUILT_SOURCES = icpyacc.h

AM_YFLAGS = -d

