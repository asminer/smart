
# $Id$

noinst_PROGRAMS = \
  sieve \
  istest

AM_CPPFLAGS = -I../src

sieve_SOURCES = sieve.cc
sieve_LDADD = ../src/libintset.la

istest_SOURCES = istest.cc
istest_LDADD = ../src/libintset.la

TESTS = \
	check00.ist \
  check01.ist \
  check02.ist \
  check03.ist 

LOG_COMPILER = ./istest

