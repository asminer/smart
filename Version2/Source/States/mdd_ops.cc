
// $Id$

#include "mdd_ops.h"

// ************************************************************
// *                   binary_cache methods                   *
// ************************************************************

binary_cache::binary_cache(node_manager* m)
{
  mdd = m; 
  heapsize = 1024;
  nodeheap = (node*) malloc(heapsize * sizeof(node));
  lastnode = -1;
  unused_nodes = -1;
  pings = hits = 0;
  table = new HashTable <binary_cache> (this);
}

binary_cache::~binary_cache()
{
  delete table;
  free(nodeheap);
}

int binary_cache::NewNode()
{
  if (unused_nodes>=0) {
    int a = unused_nodes;
    unused_nodes = nodeheap[a].next;
    return a;  
  }
  lastnode++;
  if (lastnode==heapsize) {
    heapsize += 1024;
    nodeheap = (node*) realloc(nodeheap, heapsize * sizeof(node));
    if (NULL==nodeheap)
      OutOfMemoryError("MDD cache overflow");
  }
  return lastnode;
}

// ************************************************************
// *                    operations methods                    *
// ************************************************************

operations::operations(node_manager* m)
{
  mdd = m;
  union_cache = new binary_cache(mdd);
  count_cache = new binary_cache(mdd);
}

operations::~operations()
{
  delete union_cache;
  delete count_cache;
}

int operations::Union(int a, int b)
{
  // special cases
  if (a==1 || b==1) return 1;
  if (a==0 || a==b) {
    mdd->Link(b);
    return b;
  }
  if (b==0) {
    mdd->Link(a);
    return a;
  }
  if (a>b) SWAP(a, b);
  int c;
  // check cache
  if (union_cache->Hit(a, b, c)) {
    mdd->Link(c);
    return c;
  }
  DCASSERT(mdd->NodeLevel(a) == mdd->NodeLevel(b));
  int k = mdd->NodeLevel(a);

  const int* adown = mdd->NodeData(a);
  const int* bdown = mdd->NodeData(b);
  
  int csz;
  if (mdd->isNodeSparse(a)) {
    // a is sparse
    if (mdd->isNodeSparse(b)) {
      // b is sparse
      csz = MAX(adown[2*mdd->nnzOf(a)-2], bdown[2*mdd->nnzOf(b)-2]);
      c = mdd->TempNode(k, csz);
      // Copy nonzeroes of a to c
      for (int z = mdd->nnzOf(a)-1; z>=0; z--) {
        mdd->Link(adown[2*z+1]);
        mdd->SetArc(c, adown[2*z], adown[2*z+1]);
      }
      // Union nonzeroes of b
      const int* cdown = mdd->NodeData(c);
      for (int z = 0; z<mdd->nnzOf(b); z++) {
        int i = bdown[2*z];
        mdd->SetArc(c, i, Union(cdown[i], bdown[2*z+1]));  
      }
      // done sparse-sparse
    } else {
      // b is full
      csz = MAX(mdd->SizeOf(b), adown[2*mdd->nnzOf(a)-2]); 
      c = mdd->TempNode(k, csz);
      // Copy b to c
      for (int i = mdd->SizeOf(b)-1; i>=0; i--) {
        mdd->Link(bdown[i]);
        mdd->SetArc(c, i, adown[i]);
      }
      // Union nonzeroes of a
      const int* cdown = mdd->NodeData(c);
      for (int z = 0; z<mdd->nnzOf(a); z++) {
        int i = adown[2*z];
        mdd->SetArc(c, i, Union(cdown[i], adown[2*z+1]));  
      }
      // done sparse-full
    }
  } else {
    // a is full
    if (mdd->isNodeSparse(b)) {
      // b is sparse
      csz = MAX(mdd->SizeOf(a), bdown[2*mdd->nnzOf(b)-2]); 
      c = mdd->TempNode(k, csz);
      // Copy a to c
      for (int i = mdd->SizeOf(a)-1; i>=0; i--) {
        mdd->Link(adown[i]);
        mdd->SetArc(c, i, adown[i]);
      }
      // Union nonzeroes of b
      const int* cdown = mdd->NodeData(c);
      for (int z = 0; z<mdd->nnzOf(b); z++) {
        int i = bdown[2*z];
        mdd->SetArc(c, i, Union(cdown[i], bdown[2*z+1]));  
      }
      // done full-sparse
    } else {
      // b is full
      csz = MAX(mdd->SizeOf(a), mdd->SizeOf(b));
      c = mdd->TempNode(k, csz);
      // overlapping part of a and b
      for (int i = MIN(mdd->SizeOf(a), mdd->SizeOf(b))-1; i>=0; i--) {
        mdd->SetArc(c, i, Union(adown[i], bdown[i])); 
      } // for i
      // When a is shorter than b
      for (int i = mdd->SizeOf(a); i<mdd->SizeOf(b); i++) {
        mdd->Link(bdown[i]); 
        mdd->SetArc(c, i, bdown[i]);
      }
      // When b is shorter than a
      for (int i = mdd->SizeOf(b); i<mdd->SizeOf(a); i++) {
        mdd->Link(adown[i]);
        mdd->SetArc(c, i, adown[i]);
      }
      // done full-full
    }
  } 
  
  // common to all
  c = mdd->Reduce(c);
  union_cache->Add(a, b, c);
  return c;
}

int operations::Count(int a)
{
  if (a<2) return a;
  int c;
  if (count_cache->Hit(a, a, c)) return c;
  c = 0;
  const int* adown = mdd->NodeData(a);
  if (mdd->isNodeSparse(a)) {
    for (int i = mdd->nnzOf(a)-1; i>=0; i--) {
      c += Count(adown[2*i+1]);
    }
  } else {
    for (int i = mdd->SizeOf(a)-1; i>=0; i--) {
      c += Count(adown[i]);
    }
  }
  count_cache->Add(a, a, c);
  return c;
}

