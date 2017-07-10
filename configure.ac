#
# Process this file with autoconf to produce a configure script.
# See the README file for more details.
#

AC_PREREQ(2.59)
AC_INIT([smartpack], [3.3.2], [asminer@iastate.edu], [smartpack], [http://smart.cs.iastate.edu])
AM_INIT_AUTOMAKE([-Wall foreign parallel-tests color-tests subdir-objects])
# Allow silent builds, and make it the default, if we can:
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR([src/README.txt])
AC_CONFIG_MACRO_DIR([m4])

# Check for programs
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX(11)
AM_PROG_LEX
AC_PROG_YACC
# Use AM_PROG_AR if we can...
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AM_PROG_LIBTOOL

#
# Check for GMP
#
AC_ARG_WITH([gmp],
  [AS_HELP_STRING([--without-gmp], [disable support for gmp])],
  [],
  [with_gmp=yes])
# LIBGMP=
AS_IF(
  [test "x$with_gmp" != xno], 
    [AC_CHECK_LIB([gmp], [__gmpz_init], [], [AC_MSG_FAILURE(["gmp test failed (--without-gmp to disable)"])], [])],
  [])
AM_CONDITIONAL([LINK_GMP], [test "x$with_gmp" != xno])

#
# Check for MDD Library
#
AC_ARG_WITH([meddly],
  [AS_HELP_STRING([--without-meddly], [disable support for Meddly])],
  [],
  [with_meddly=yes])
AS_IF([test "x$with_meddly" != xno],
  [AC_CHECK_HEADERS([meddly.h], [],
    [AC_MSG_FAILURE(["Couldn't find MDD Library header (--without-meddly to disable)"])], [])],
  [])
AM_CONDITIONAL([LINK_MEDDLY], [test "x$with_meddly" != xno])

# Check for header files.
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([sys/time.h unistd.h])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_TYPE_SIGNAL

# Our configuration stuff

AC_SUBST(MDD_LIBRARY)

# Makefiles we need to build

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/_LSLib/Makefile
                 src/_LSLib/examples/Makefile
                 src/_RngLib/Makefile
                 src/_RngLib/examples/Makefile
                 src/_SimLib/Makefile
                 src/_SimLib/examples/Makefile
                 src/_IntSets/Makefile
                 src/_IntSets/examples/Makefile
                 src/_StateLib/Makefile
                 src/_StateLib/examples/Makefile
                 src/_GraphLib/Makefile
                 src/_GraphLib/examples/Makefile
                 src/_MCLib/Makefile
                 src/_MCLib/examples/Makefile
                 Tests/Makefile
                 ])


AC_OUTPUT

