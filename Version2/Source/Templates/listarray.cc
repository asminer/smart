
// $Id$

/*
    Implementation of listarray_base
*/



#include "listarray.h"

#include "../Base/memtrack.h"

//#define DEBUG_LISTS

// ******************************************************************
// *                                                                *
// *                     listarray_base  methods                    *
// *                                                                *
// ******************************************************************

listarray_base::listarray_base()
{
  ALLOC("listarray_base", sizeof(listarray_base));
  num_lists = 0;
  num_items = 0;
  list_pointer = NULL;
  lists_alloc = 0;
  next = NULL;
  items_alloc = 0;
  isDynamic = true;
}

listarray_base::~listarray_base()
{
  FREE("listarray_base", sizeof(listarray_base));
  isDynamic = true;
  free(list_pointer);
  list_pointer = NULL;
  free(next);
  next = NULL;
}

void listarray_base::ResizeLists(int new_nodes) 
{
  DCASSERT(IsDynamic());
  if (lists_alloc != new_nodes) {
      int* foo = (int *) realloc(list_pointer, (1+new_nodes)*sizeof(int));
      if (new_nodes && (NULL==foo)) OutOfMemoryError("Lists resize");
      list_pointer = foo;
      lists_alloc = new_nodes;
  }
}

void listarray_base::ConvertToDynamic()
{
  if (IsDynamic()) return;

  // Add next array
  next = (int *) realloc(next, items_alloc*sizeof(int));
  if (items_alloc && (NULL==next)) OutOfMemoryError("Lists conversion");

  // Fill next array, set list pointers to list *tails*
  int s;
  for (s=0; s<num_lists; s++) {
    if (list_pointer[s] == list_pointer[s+1]) {
      // empty row
      list_pointer[s] = -1;
      continue;
    }
    int e;
    for (e=list_pointer[s]; e<list_pointer[s+1]-1; e++) {
      next[e] = e+1;
    }
    e = list_pointer[s+1]-1;
    next[e] = list_pointer[s];
    list_pointer[s] = e;
  }
  list_pointer[num_lists] = -1;
  isDynamic = true;
}

