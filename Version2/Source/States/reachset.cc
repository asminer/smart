
// $Id$

#include "reachset.h"

/** @name reachset.cc
    @type File
    @args \ 

    Simple, single class for "compacted" reachability sets.
    
 */

//@{ 

// ==================================================================
// ||                                                              ||
// ||                       reachset methods                       ||
// ||                                                              ||
// ==================================================================

reachset::reachset()
{
  size = 0;
  encoding = RT_None;
}

reachset::~reachset()
{
  switch (encoding) {
    case RT_Explicit:
	delete flat;
	break;

    case RT_Implicit:
 	// destroy evmdd here
	break;

    default:
	// do nothing (but keep compiler happy ;^)
	break;
  }
}

void reachset::CreateEnumerated(int s)
{
  DCASSERT(encoding == RT_None);
  encoding = RT_Enumerated;
  size = s;
}

void reachset::CreateExplicit(flatss* f)
{
  DCASSERT(encoding == RT_None);
  encoding = RT_Explicit;
  flat = f;
}

void reachset::CreateImplicit(void* e)
{
  DCASSERT(encoding == RT_None);
  encoding = RT_Implicit;
  evmdd = e;
}

void reachset::CreateError()
{
  DCASSERT(encoding == RT_None);
  encoding = RT_Error;
}

//@}

