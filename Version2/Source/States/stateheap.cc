
// $Id$

#include "stateheap.h"

void state::Show(OutputStream& s) const
{
  int i;
  s.Put('[');
#ifdef EXTRACTED_FROM_SMART
  s.Put(data[0].ivalue);
#else
  PrintResult(s, INT, data[0]);
#endif
  for (i=1; i<size; i++) {
    s.Put(", ");
#ifdef EXTRACTED_FROM_SMART
    s.Put(data[i].ivalue);
#else
    PrintResult(s, INT, data[i]); 
#endif
  }
  s.Put(']');
}

/*
	Someday, we may want to set up a nice memory heap
  	just for states to speed things up a bit.

	For now, Andy is lazy, so...
*/

bool AllocState(state &s, int length)
{
  s.am_substate = false;
  s.size = length;
  s.data = new result[length];
  return true;
}

void FreeState(state &s)
{
  if (s.am_substate) {
    // do NOT delete anything
    s.size = 0;
    s.data = NULL;
    return;
  }
  delete[] s.data;
  s.size = 0;
  s.data = NULL;
}

void MakeSubstate(state &sub, const state &s, int pos, int len)
{
  if ((pos<0) || (pos+len > s.size)) {
    // range error
    sub.size = 0;
    sub.data = NULL;
  } else {
    sub.am_substate = true;
    sub.size = len;
    sub.data = s.data + pos;
  }
}

