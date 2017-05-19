#
# $Id$
#
# The configure script is built based on this file.
# See the README / INSTALL file for more details.
#

AC_PREREQ(2.59)
AC_INIT([smartlibs], [3], [asminer@iastate.edu])
AM_INIT_AUTOMAKE([-Wall foreign parallel-tests color-tests])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR([m4])
AC_PREFIX_DEFAULT([$PWD])

#
# Check for programs that we need
#
AC_PROG_CXX
# AM_PROG_AR
AM_PROG_LIBTOOL

#
# Check for header files we need
#
AC_CHECK_HEADERS([sys/time.h unistd.h])

#
# Check for important library functions
#
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC

#
# Check for PNFront library
#
AC_ARG_WITH([pnfront-path],
  [AS_HELP_STRING([--with-pnfront-path], [location of PNFront library])],
  [CPPFLAGS="$CPPFLAGS -I$withval/include"; PNFRONT_LIB="$withval/lib/libpnfront.a"],
  [])
AC_SUBST([PNFRONT_LIB])

AC_ARG_WITH([pnfront],
  [AS_HELP_STRING([--without-pnfront], [disable use of PNFront library])],
  [],
  [with_pnfront=yes])

AC_LANG([C++])
LIBPNFRONT=
AS_IF([test "x$with_pnfront" != xno],
  [AC_CHECK_HEADERS([pnfront.h], [],
    [AC_MSG_FAILURE([Could not find PNFront header, read INSTALL file for options])],
    []
  )])


#
# Makefiles we need to build
#
AC_CONFIG_FILES([Makefile
                 Timer/Makefile
                 Timer/src/Makefile
                 RngLib/Makefile
                 RngLib/src/Makefile
                 RngLib/examples/Makefile
                 SimLib/Makefile
                 SimLib/src/Makefile
                 SimLib/examples/Makefile
                 IntSets/Makefile
                 IntSets/src/Makefile
                 IntSets/examples/Makefile
                 GraphLib/Makefile
                 GraphLib/src/Makefile
                 GraphLib/examples/Makefile
                 LSLib/Makefile
                 LSLib/src/Makefile
                 LSLib/examples/Makefile
                 MCParse/Makefile
                 MCParse/src/Makefile
                 MCParse/examples/Makefile
                 MCLib/Makefile
                 MCLib/src/Makefile
                 MCLib/examples/Makefile
                 StateLib/Makefile
                 StateLib/src/Makefile
                 StateLib/examples/Makefile
                ])

AC_OUTPUT
