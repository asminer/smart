
// $Id$

#include "functions.h"
//@Include: functions.h

/** @name symbpls.cc
    @type File
    @args \ 

   Implementation of functions with parameters.

 */

//@{

/// The Run-time stack
void** ParamStack;
/// Top of the run-time stack
int ParamStackTop;
/// Size of the Run-time stack
int ParamStackSize;

// ******************************************************************
// *                                                                *
// *                      formal_param methods                      *
// *                                                                *
// ******************************************************************

formal_param::formal_param(const char* fn, int line, type t, char* n)
  : symbol(fn, line, t, n)
{
  pass = NULL;
  deflt = NULL;
}

formal_param::~formal_param()
{
  delete pass;
  delete deflt;
}

void formal_param::show(ostream &s) const
{
  if (NULL==Name()) return;  // Don't show hidden parameters

  s << GetType(Type()) << " " << Name() << " := " << pass;
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

void CreateRuntimeStack(int size)
{
  ParamStack = new void* [size];
  ParamStackSize = size;
  ParamStackTop = 0;
}

void DestroyRuntimeStack()
{
  DCASSERT(ParamStackTop==0);  // Otherwise we're mid function call!
  delete[] ParamStack;
  ParamStack = NULL;
}

void DumpRuntimeStack(ostream &s)
{
  s << "Sorry, DumpRuntimeStack is not yet implemented.\n";
  s << "But some day, it will be very, very slick.\n";
}


//@}

