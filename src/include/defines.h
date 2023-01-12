
/** @name defines.h
    @type File
    @args \

    The base of all modules.  So if you change this, everything gets
    to recompile.

    This file is for good global defines, such as DCASSERT.

    Since this file is only intended for global definitions, there is no
    associated defines.c or defines.cc file.

 */

//@{

#ifndef DEFINES_H
#define DEFINES_H

#include <assert.h>

// Handy Macros

/// Standard MAX "macro".
template <class T> inline T MAX(T X,T Y) { return ((X>Y)?X:Y); }
/// Standard MIN "macro".
template <class T> inline T MIN(T X,T Y) { return ((X<Y)?X:Y); }
/// Standard ABS "macro".
template <class T> inline T ABS(T X) { return ((X<0)?(-X):(X)); }
/// SWAP "macro".
template <class T> inline void SWAP(T &x, T &y) { T tmp=x; x=y; y=tmp; }
/// POSITIVE "macro".
template <class T> inline bool POSITIVE(T X) { return (X>0) ? 1 : 0; }
/// SIGN "macro".
template <class T> inline int SIGN(T X) { return (X<0) ? -1 : POSITIVE(X); }

/// Euclid's GCD algorithm
//
inline long GCD(long a, long b)
{
  long r = a % b;
  while (r > 0) {
    a = b;
    b = r;
    r = a % b;
  }
  return b;
}


/*
    There are now two modes of code generation:
       "DEVELOPMENT_CODE" and "RELEASE_CODE".

    If "DEVELOPMENT_CODE" is defined (usually done in the makefile) then
    debugging macros and assertions will be turned on.  Otherwise we assume
    that we have "RELEASE_CODE" and they are turned off.

    Macros useful for debugging "development code" that are turned off
    for release code (for speed):

    DCASSERT()
    CHECK_RANGE(low, x, high+1)
*/
/// Omega
const int OOmega=-10;

#ifdef DEVELOPMENT_CODE
  #define MEM_TRACE_ON
  #define RANGE_CHECK_ON
  #define DCASSERTS_ON
  #include <stdio.h>
#endif


// Use this for assertions that will fail only when your
// code is wrong.  Handy for debugging.
#ifdef DCASSERTS_ON
#define DCASSERT(X) assert(X)
#else
#define DCASSERT(X)
#endif

// Also useful for debugging.
inline void CHECK_RANGE(const char* fn, unsigned ln,
        long min, long value, long max)
{
#ifdef RANGE_CHECK_ON
    if (value >= max || value < min) {
        fprintf(stderr, "Check range at %s line %u failed:\n", fn, ln);
        fprintf(stderr, "    min: %ld\n    val: %ld\n    max: %ld\n",
                min, value, max);
        assert(0);
    }
#endif
}

// More debugging tricks
#ifdef DEVELOPMENT_CODE
  #define CHECK_RETURN(X, V)  DCASSERT(V == X)
#else
  #define CHECK_RETURN(X, V)  X
#endif

// Safe typecasting for development code;  fast casting otherwise

#ifdef DEVELOPMENT_CODE
#define smart_cast  dynamic_cast
#else
#define smart_cast  static_cast
#endif

// Flip this on for malloc / free errors

// #define DEBUG_MEM

#endif

//@}
