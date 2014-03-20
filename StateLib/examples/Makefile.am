
# $Id$

noinst_PROGRAMS = \
  enum1 \
  enum2 \
  dbtest \
  statediff \
  testcoll \
  pngen

AM_CPPFLAGS = -I../src 

enum1_SOURCES = enum1.cc timers.h timers.cc
enum1_LDADD = ../src/libstate.la

enum2_SOURCES = enum2.cc timers.h timers.cc
enum2_LDADD = ../src/libstate.la

dbtest_SOURCES = dbtest.cc
dbtest_LDADD = ../src/libstate.la

statediff_SOURCES = statediff.cc 
statediff_LDADD = ../src/libstate.la

testcoll_SOURCES = testcoll.cc
testcoll_LDADD = ../src/libstate.la

pngen_SOURCES = pngen.cc timers.h timers.cc
pngen_LDADD = ../src/libstate.la $(PNFRONT_LIB)

TESTS = \
  dbin1.txt \
  dbin2.txt 

LOG_COMPILER = ./CheckScript
