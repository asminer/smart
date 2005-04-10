
// $Id$

/*
    Splay implementation for non-template base classes.
*/

#include "../Base/streams.h"
#include "splay.h"
#include "../Base/memtrack.h"

ArraySplayBase::ArraySplayBase()
{
  ALLOC("ArraySplayBase", sizeof(ArraySplayBase));
  left = NULL;  
  right = NULL;
  nodes_alloc = 0;
  nodes_used = 0;
  root = -1;
  path = new Stack <int> (16);
}

ArraySplayBase::~ArraySplayBase()
{
  FREE("ArraySplayBase", sizeof(ArraySplayBase));
  free(left);
  free(right);
  delete path;
}

void ArraySplayBase::FillOrderList(int tree, int &index, int* order)
{
  // Nice, inorder traversal of the tree
  if (tree<0) return;
  CHECK_RANGE(0, tree, nodes_used);
  FillOrderList(left[tree], index, order);
  order[index++] = tree;
  FillOrderList(right[tree], index, order);
}

