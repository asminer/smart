
# $Id$

noinst_PROGRAMS = \
  enum1 \
  enum2 \
  dbtest \
  statediff \
  testcoll \
  pngen

AM_CPPFLAGS = -I../src -I../../Timer/src

enum1_SOURCES = enum1.cc 
enum1_LDADD = ../src/libstate.la ../../Timer/src/libtimer.la

enum2_SOURCES = enum2.cc 
enum2_LDADD = ../src/libstate.la ../../Timer/src/libtimer.la

dbtest_SOURCES = dbtest.cc
dbtest_LDADD = ../src/libstate.la

statediff_SOURCES = statediff.cc 
statediff_LDADD = ../src/libstate.la

testcoll_SOURCES = testcoll.cc
testcoll_LDADD = ../src/libstate.la

pngen_SOURCES = pngen.cc 
pngen_LDADD = ../src/libstate.la $(PNFRONT_LIB) ../../Timer/src/libtimer.la

TESTS = \
  dbin1.txt \
  dbin2.txt 

LOG_COMPILER = ./CheckScript
