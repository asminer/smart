
// $Id$

/** @name defines.h
    @type File
    @args \ 

  The base of all smart files.  So if you change this, everything gets
  to recompile.

  This file is for good global defines, such as ASSERT and TRACE and crud.

  Since this file is only intended for global definitions, there is no
  associated defines.c or defines.cc file.

  Note: no more exec trace.  Was it ever used?

 */

//@{

#ifndef DEFINES_H
#define DEFINES_H

// Things that everyone will need
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define _VERSION "2.0"

// Handy Constants

const int NOT_FOUND = -1;
const int UNKNOWN = -1;

// Exit codes

const int NORMAL_EXIT = 0;
const int FATAL_ERROR = 1;
const int INTERNAL_ERROR = 2;
const int SMART_PANIC = 3;

// Handy Macros

/// Standard MAX "macro".
template <class T> inline T MAX(T X,T Y) { return ((X>Y)?X:Y); }
/// Standard MIN "macro".
template <class T> inline T MIN(T X,T Y) { return ((X<Y)?X:Y); }
/// Standard ABS "macro".
template <class T> inline T ABS(T X) { return ((X<0)?(-X):(X)); }

/// SWAP "macro".
template <class T> inline void SWAP(T &x, T &y) { T tmp=x; x=y; y=tmp; }

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

#ifdef DEVELOPMENT_CODE
  #define MEM_TRACE_ON  
  #define RANGE_CHECK_ON
  #define DCASSERTS_ON
#endif


// Use this for assertions that you always check for
#define ASSERT(X) assert(X)

// Use this for assertions that will fail only when your
// code is wrong.  Handy for debugging.
#ifdef DCASSERTS_ON 
#define DCASSERT(X) assert(X)
#else
#define DCASSERT(X)
#endif

// Helpful for finding memory leaks
#ifdef MEM_TRACE_ON
#define MEM_ALLOC(LEV, SIZE, DESC) if (MemTrace) MemTrace->Alloc(LEV, SIZE, DESC);
#define MEM_FREE(LEV, SIZE, DESC) if (MemTrace) MemTrace->Free(LEV, SIZE, DESC);
#else
#define MEM_ALLOC(LEV, SIZE, DESC) 
#define MEM_FREE(LEV, SIZE, DESC) 
#endif

// Also useful for debugging.
#ifdef RANGE_CHECK_ON
  inline void CheckRange(int min, int value, int max)
  {
    ASSERT(value<max);
    ASSERT(value>=min);
  }
  #define CHECK_RANGE(MIN, VALUE, MAX)  CheckRange(MIN, VALUE, MAX)
#else
  #define CHECK_RANGE(MIN, VALUE, MAX)
#endif

#endif

//@}

