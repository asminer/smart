
# $Id$

noinst_PROGRAMS = \
  axzero \
  axb \
  phase

AM_CPPFLAGS = -I../src -I../../Timer/src

axzero_SOURCES = axzero.cc matrix.h matrix.cc
axzero_LDADD = ../src/libls.la ../../Timer/src/libtimer.la

axb_SOURCES = axb.cc matrix.h matrix.cc
axb_LDADD = ../src/libls.la ../../Timer/src/libtimer.la

phase_SOURCES = phase.cc 
phase_LDADD = ../src/libls.la
