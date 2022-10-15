
noinst_PROGRAMS = \
  dice \
  oz_test \
  bssq_test \
  sms_test

AM_CPPFLAGS = -I$(srcdir)/.. -I$(srcdir)/../../_RngLib

dice_SOURCES = dice.cc 
dice_LDADD = ../libsim.la

oz_test_SOURCES = oz_test.cc tester.h tester.cc 
oz_test_LDADD = ../libsimmdl.la ../../_RngLib/librng.la

bssq_test_SOURCES = bssq_test.cc tester.h tester.cc
bssq_test_LDADD = ../libsimmdl.la ../../_RngLib/librng.la

sms_test_SOURCES = sms_test.cc tester.h tester.cc
sms_test_LDADD = ../libsimmdl.la ../../_RngLib/librng.la

