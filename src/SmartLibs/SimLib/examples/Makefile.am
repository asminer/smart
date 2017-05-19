
# $Id$

noinst_PROGRAMS = \
  dice \
  oz_test \
  bssq_test \
  sms_test

AM_CPPFLAGS = -I../src -I../../RngLib/src

dice_SOURCES = dice.cc
dice_LDADD = ../src/libsim.la

oz_test_SOURCES = oz_test.cc tester.h tester.cc
oz_test_LDADD = ../src/libsimmdl.la ../../RngLib/src/librng.la

bssq_test_SOURCES = bssq_test.cc tester.h tester.cc
bssq_test_LDADD = ../src/libsimmdl.la ../../RngLib/src/librng.la

sms_test_SOURCES = sms_test.cc tester.h tester.cc
sms_test_LDADD = ../src/libsimmdl.la ../../RngLib/src/librng.la

