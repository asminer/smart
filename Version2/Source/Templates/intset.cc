
// $Id$

#include "intset.h"
#include "../Base/errors.h"

int_set::int_set() 
{
  next = NULL;
  last = size = 0;
  head = tail = -1; 
}

int_set::~int_set()
{
  free(next);
}

void int_set::EnlargeData(int sz)
{
  int dsize = (1+sz/16)*16;
  next = (int*) realloc(next, dsize*sizeof(int));
  if (0==next)
	OutOfMemoryError("Integer set enlargement");
  for (; size<dsize; size++) next[size] = -1;
}

