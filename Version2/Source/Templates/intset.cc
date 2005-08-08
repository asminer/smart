
// $Id$

#include "intset.h"
#include "../Base/errors.h"

int_set::int_set() 
{
  data = 0;
  last = 0;
  dsize = 0;
  
  contains = 0;
  csize = 0;
}

int_set::~int_set()
{
  free(data);
  free(contains);
}

void int_set::SetMax(int n)
{
  if (n<csize) return;
  int lastsize = csize;
  csize = (1+n/16)*16;
  contains = (bool*) realloc(contains, csize * sizeof(bool));
  if (0==contains) 
	OutOfMemoryError("Integer set enlargement");

  for (; lastsize<csize; lastsize++)
	contains[lastsize] = false; 
}

void int_set::EnlargeData()
{
  dsize += 16;
  data = (int*) realloc(data, dsize*sizeof(int));
  if (0==data)
	OutOfMemoryError("Integer set enlargement");
}
