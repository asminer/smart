
noinst_PROGRAMS = \
  enum1 \
  enum2 \
  dbtest \
  statediff \
  testcoll 

AM_CPPFLAGS = -I$(srcdir)/../ -I$(srcdir)/../../_Timer/

enum1_SOURCES = enum1.cc ../../_Timer/timerlib.cc
enum1_LDADD = ../libstate.la 

enum2_SOURCES = enum2.cc ../../_Timer/timerlib.cc
enum2_LDADD = ../libstate.la 

dbtest_SOURCES = dbtest.cc 
dbtest_LDADD = ../libstate.la

statediff_SOURCES = statediff.cc 
statediff_LDADD = ../libstate.la

testcoll_SOURCES = testcoll.cc
testcoll_LDADD = ../libstate.la

# TESTS = \
#  dbin1.txt \
#  dbin2.txt 

# LOG_COMPILER = ./CheckScript
