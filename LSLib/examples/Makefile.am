
# $Id$

noinst_PROGRAMS = \
  axzero \
  axb \
  phase

AM_CPPFLAGS = -I../src

axzero_SOURCES = axzero.cc timers.h timers.cc matrix.h matrix.cc
axzero_LDADD = ../src/libls.la

axb_SOURCES = axb.cc timers.h timers.cc matrix.h matrix.cc
axb_LDADD = ../src/libls.la

phase_SOURCES = phase.cc 
phase_LDADD = ../src/libls.la
