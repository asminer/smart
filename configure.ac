# $Id$
#
#
# Process this file with autoconf to produce a configure script.
# See the README file for more details.
#

AC_PREREQ(2.59)
AC_INIT([smartpack], [3.2], [asminer@iastate.edu], [smartpack], [http://smart.cs.iastate.edu])
AM_INIT_AUTOMAKE([-Wall foreign parallel-tests color-tests])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR([src/README.txt])
AC_CONFIG_MACRO_DIR([m4])

# Check for programs
AC_PROG_CXX
AC_PROG_LEX
AC_PROG_YACC
AM_PROG_AR
AM_PROG_LIBTOOL

#
# Check for GMP
#
AC_ARG_WITH([gmp],
  [AS_HELP_STRING([--without-gmp], [disable support for gmp])],
  [],
  [with_gmp=yes])
LIBGMP=
AS_IF([test "x$with_gmp" != xno],
  [AC_CHECK_LIB([gmp], [__gmpz_init], [],
    [AC_MSG_FAILURE([gmp test failed (--without-gmp to disable)])],
    [])])

# Check for header files.
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([sys/time.h unistd.h])

#
# Check for SimLib
#
AC_CHECK_HEADER([sim.h], [],
    [AC_MSG_FAILURE(["Couldn't find SimLib header"])],
    [])

#
# Check for RngLib
#
AC_CHECK_HEADER([rng.h], [],
    [AC_MSG_FAILURE(["Couldn't find RngLib header"])],
    [])

#
# Check for LSLib
#
AC_CHECK_HEADER([lslib.h], [],
    [AC_MSG_FAILURE(["Couldn't find LSLib header"])],
    [])

#
# Check for StateLib
#
AC_CHECK_HEADER([statelib.h], [],
    [AC_MSG_FAILURE(["Couldn't find StateLib header"])],
    [])

#
# Check for GraphLib
#
AC_CHECK_HEADER([graphlib.h], [],
    [AC_MSG_FAILURE(["Couldn't find GraphLib header"])],
    [])

#
# Check for MCLib
#
AC_CHECK_HEADER([mclib.h], [],
    [AC_MSG_FAILURE(["Couldn't find MCLib header"])],
    [])

#
# Check for IntSets
#
AC_CHECK_HEADER([intset.h], [],
    [AC_MSG_FAILURE(["Couldn't find IntSets header"])],
    [])

#
# Check for MDD Library
#
AC_CHECK_HEADER([meddly.h], [],
    [AC_MSG_FAILURE(["Couldn't find MDD Library header"])],
    [])


# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_TYPE_SIGNAL

# Our configuration stuff

AC_SUBST(MDD_LIBRARY)
AC_SUBST(SIM_LIBRARY)
AC_SUBST(RNG_LIBRARY)
AC_SUBST(LS_LIBRARY)
AC_SUBST(STATE_LIBRARY)
AC_SUBST(GRAPH_LIBRARY)
AC_SUBST(MC_LIBRARY)
AC_SUBST(IS_LIBRARY)

# Makefiles we need to build

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/include/Makefile
                 src/Timers/Makefile
                 src/Streams/Makefile
                 src/Options/Makefile
                 src/ExprLib/Makefile
                 src/SymTabs/Makefile
                 src/Modules/Makefile
                 src/Formlsms/Makefile
                 src/Engines/Makefile
                 src/FuncLib/Makefile
                 src/ParseSM/Makefile
                 src/ParseICP/Makefile
                 src/TestApps/Makefile
                 src/Apps/Makefile
                 Tests/Makefile
                 ])


AC_OUTPUT

