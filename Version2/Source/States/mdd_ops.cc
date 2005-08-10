
// $Id$

#include "mdd_ops.h"

#define UNION_TRACE
#define FIRE_TRACE
#define SATURATE_TRACE

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
  fire_cache = new binary_cache(mdd);
  counts = NULL;
  countsize = 0;
}

operations::~operations()
{
  delete union_cache;
  delete fire_cache;
  free(counts);
}

int operations::Union(int a, int b)
{
  DCASSERT(mdd->Incount(a));
  DCASSERT(mdd->Incount(b));
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
#ifdef UNION_TRACE
    Output << "Union(" <<a<< ", " <<b<< ") in cache, result " <<c<< "\n";
    Output.flush();
#endif
    mdd->Link(c);
    return c;
  }
  DCASSERT(mdd->NodeLevel(a) == mdd->NodeLevel(b));
  int k = mdd->NodeLevel(a);
#ifdef UNION_TRACE
  Output << "Starting union of ";
  Output.Put(a, 5);
  Output.Put(' ');
  mdd->ShowNode(Output, a);
  Output << "\n     and          ";
  Output.Put(b, 5);
  Output.Put(' ');
  mdd->ShowNode(Output, b);
  Output << "\n\n";
  Output.flush();
#endif

  int csz;
  if (mdd->isNodeSparse(a)) {
    // a is sparse
    if (mdd->isNodeSparse(b)) {
      // b is sparse
      csz = MAX(mdd->Data(a, 2*mdd->nnzOf(a)-2), 
		mdd->Data(b, 2*mdd->nnzOf(b)-2));
      c = mdd->TempNode(k, csz);
      // Copy nonzeroes of a to c
      for (int z = mdd->nnzOf(a)-1; z>=0; z--) {
 	int d = mdd->Data(a, 2*z+1);
        mdd->Link(d);
        mdd->SetArc(c, mdd->Data(a, 2*z), d);
      }
      // Union nonzeroes of b
      for (int z = 0; z<mdd->nnzOf(b); z++) {
        int i = mdd->Data(b, 2*z);
        mdd->SetArc(c, i, Union(mdd->Data(c, i), mdd->Data(b, 2*z+1)));  
      }
      // done sparse-sparse
    } else {
      // b is full
      csz = MAX(mdd->SizeOf(b), mdd->Data(a, 2*mdd->nnzOf(a)-2)); 
      c = mdd->TempNode(k, csz);
      // Copy b to c
      for (int i = mdd->SizeOf(b)-1; i>=0; i--) {
	int d = mdd->Data(b, i);
        mdd->Link(d);
        mdd->SetArc(c, i, d);
      }
      // Union nonzeroes of a
      for (int z = 0; z<mdd->nnzOf(a); z++) {
        int i = mdd->Data(a, 2*z);
        mdd->SetArc(c, i, Union(mdd->Data(c, i), mdd->Data(a, 2*z+1)));  
      }
      // done sparse-full
    }
  } else {
    // a is full
    if (mdd->isNodeSparse(b)) {
      // b is sparse
      csz = MAX(mdd->SizeOf(a), mdd->Data(b, 2*mdd->nnzOf(b)-2)); 
      c = mdd->TempNode(k, csz);
      // Copy a to c
      for (int i = mdd->SizeOf(a)-1; i>=0; i--) {
	int d = mdd->Data(a, i);
        mdd->Link(d);
        mdd->SetArc(c, i, d);
      }
      // Union nonzeroes of b
      for (int z = 0; z<mdd->nnzOf(b); z++) {
        int i = mdd->Data(b, 2*z);
        mdd->SetArc(c, i, Union(mdd->Data(c, i), mdd->Data(b, 2*z+1)));  
      }
      // done full-sparse
    } else {
      // b is full
      csz = MAX(mdd->SizeOf(a), mdd->SizeOf(b));
      c = mdd->TempNode(k, csz);
      // overlapping part of a and b
      for (int i = MIN(mdd->SizeOf(a), mdd->SizeOf(b))-1; i>=0; i--) {
        mdd->SetArc(c, i, Union(mdd->Data(a, i), mdd->Data(b, i))); 
      } // for i
      // When a is shorter than b
      for (int i = mdd->SizeOf(a); i<mdd->SizeOf(b); i++) {
	int d = mdd->Data(b, i);
        mdd->Link(d); 
        mdd->SetArc(c, i, d);
      }
      // When b is shorter than a
      for (int i = mdd->SizeOf(b); i<mdd->SizeOf(a); i++) {
        int d = mdd->Data(a, i);
        mdd->Link(d);
        mdd->SetArc(c, i, d);
      }
      // done full-full
    }
  } 
  
  // common to all
  c = mdd->Reduce(c);
  union_cache->Add(a, b, c);
#ifdef UNION_TRACE
  Output << "Union of ";
  Output.Put(a, 5);
  Output.Put(' ');
  mdd->ShowNode(Output, a);
  Output << "\n     and ";
  Output.Put(b, 5);
  Output.Put(' ');
  mdd->ShowNode(Output, b);
  Output << "\nResult   ";
  Output.Put(c, 5);
  Output.Put(' ');
  mdd->ShowNode(Output, c);
  Output << "\n\n";
  Output.flush();
#endif
  return c;
}

int operations::Count(int a)
{
  if (a<2) return a;
  if (a>=countsize) {
    int oldcs = countsize;
    countsize = (1+a/16)*16;
    counts = (int*) realloc(counts, countsize * sizeof(int));
    if (NULL==counts)
	OutOfMemoryError("MDD Counting");
    for(; oldcs<countsize; oldcs++) counts[oldcs] = 0;
  }
  if (counts[a]) return counts[a];
  if (mdd->isNodeSparse(a)) {
    for (int i = mdd->nnzOf(a)-1; i>=0; i--) {
      counts[a] += Count(mdd->Data(a, 2*i+1));
    }
  } else {
    for (int i = mdd->SizeOf(a)-1; i>=0; i--) {
      counts[a] += Count(mdd->Data(a, i));
    }
  }
  return counts[a];
}

void operations::Saturate(int init, int* r, int* s, int k)
{
  K = k;  
  roots = r;
  sizes = s;

  // allocate temp stuff
  Lset = new int_set*[K+1];
  Lset[0] = 0;
  for (k=1; k<=K; k++) 
    Lset[k] = new int_set;

  // Start saturation
  TopSaturate(init);

  // destroy temp stuff
  for (k=1; k<=K; k++) {
    delete Lset[k];
  }
  delete[] Lset; 
}

void operations::TopSaturate(int init)
{
  if (init<2) return;
  int k = mdd->NodeLevel(init);
  DCASSERT(mdd->SizeOf(init)==sizes[k]);
  for (int i=mdd->SizeOf(init)-1; i>=0; i--) 
    if (mdd->Data(init, i)) TopSaturate(mdd->Data(init, i));
  if (roots[k]) Saturate(init);
}

void operations::Saturate(int init)
{
  int k = mdd->NodeLevel(init);
  DCASSERT(k>0);  
  DCASSERT(roots[k]);

#ifdef UNION_TRACE
  Output << "Saturating " << init << " ";
  mdd->ShowNode(Output, init);
  Output << "\n\n";
  Output.flush();
#endif
  int rowlist = roots[k];
  bool rowsparse = mdd->isNodeSparse(rowlist);
  int rowsize;
  Lset[k]->SetMax(sizes[k]);

  if (rowsparse) { 
    rowsize = mdd->nnzOf(rowlist);
    for (int z=rowsize-1; z>=0; z--) {
      int i = mdd->Data(rowlist, 2*z);
      CHECK_RANGE(0, i, mdd->SizeOf(init));
      if (mdd->Data(init, i)) Lset[k]->AddElement(i);
    } // for z
  } else {
    rowsize = mdd->SizeOf(rowlist);
    DCASSERT(rowsize <= mdd->SizeOf(init));
    for (int i=rowsize-1; i>=0; i--) {
      if (mdd->Data(init, i) && mdd->Data(rowlist, i)) Lset[k]->AddElement(i);
    } // for i
  } // if rowsparse

  while (Lset[k]->Card()) {
    int cols;
    bool colsparse;
    int colsize;
    int i = Lset[k]->RemoveElement();
    if (rowsparse) {
      int z;
      for (z=rowsize-1; z>=0; z--) {
	int ndx = mdd->Data(rowlist, 2*z);
        if (ndx == i) break;
        if (ndx < i) z = 0; // will never be found
      }
      if (z<0) continue;  // not found
      cols = mdd->Data(rowlist, 2*z+1);
      colsparse = mdd->isNodeSparse(cols);
      colsize = mdd->nnzOf(cols);
    } else {
      if (i>=rowsize) continue;
      cols = mdd->Data(rowlist, i);
      if (0==cols) continue;
      colsparse = mdd->isNodeSparse(cols);
    } // if rowsparse

    if (colsparse) {
      // scan through sparse column
      for (int z=mdd->nnzOf(cols)-1; z>=0; z--) {
        int j = mdd->Data(cols, 2*z);
        int f = RecFire(mdd->Data(init, i), mdd->Data(cols, 2*z+1));
        if (f) {
	  int u = Union(f, mdd->Data(init, j));
   	  mdd->Unlink(f);
	  if (u!=mdd->Data(init, j)) {
	    mdd->SetArc(init, j, u);
	    Lset[k]->AddElement(j);
          } // if u
        } // f 
      } // for z
    } else {
      // scan through full column
      for (int j=mdd->SizeOf(cols)-1; j>=0; j--) {
        int f = RecFire(mdd->Data(init, i), mdd->Data(cols, j));
        if (f && f!=mdd->Data(init, j)) {
	  int u = Union(f, mdd->Data(init, j));
   	  mdd->Unlink(f);
	  if (u!=mdd->Data(init, j)) {
	    mdd->SetArc(init, j, u);
	    Lset[k]->AddElement(j);
          } else {
	    mdd->Unlink(mdd->Data(init, j));
          } // if u
        } // f 
      } // for j
    } // if colsparse

  } // massive while

#ifdef UNION_TRACE
  Output << "Saturated  " << init << " ";
  mdd->ShowNode(Output, init);
  Output << "\n\n";
  Output.flush();
#endif
}

bool operations::FireRow(int s, int pd, int row)
{
  if (0==row || 0==pd) return false;
#ifdef FIRE_TRACE
  Output << "Firing node " << pd << " with row " << row << " : ";
  mdd->ShowNode(Output, row);
  Output << "\n";
  Output.flush();
#endif
  bool nonzero = false;
  if (mdd->isNodeSparse(row)) {
    for (int z = mdd->nnzOf(row)-1; z>=0; z--) {
      int j = mdd->Data(row, 2*z);
      int f = RecFire(pd, mdd->Data(row, 2*z+1));
      int u = Union(mdd->Data(s, j), f);
      mdd->Unlink(f);
      mdd->SetArc(s, j, u);
      if (u) nonzero = true;
    }
  } else {
    for (int j = mdd->SizeOf(row)-1; j>=0; j--) {
      int f = RecFire(pd, mdd->Data(row, j));
      int u = Union(mdd->Data(s, j), f);
      mdd->Unlink(f);
      mdd->SetArc(s, j, u);
      if (u) nonzero = true;
    }
  }
  return nonzero;
}

int operations::RecFire(int p, int mxd)
{
  DCASSERT(mdd->Incount(p));
  DCASSERT(mdd->Incount(mxd));
  if (0==p || 0==mxd) return 0;
  if (1==mxd) {
    mdd->Link(p);
    return p;
  }
  DCASSERT(p>1);
  int s;
  if (fire_cache->Hit(p, mxd, s)) {
#ifdef FIRE_TRACE
    Output << "RecFire(" <<p<< ", " <<mxd<< ") in cache, result " <<s<< "\n";
    Output.flush();
#endif
    mdd->Link(s);
    return s;
  }
#ifdef FIRE_TRACE
  Output << "Starting RecFire node: ";
  Output.Put(p, 5);
  Output.Put(' ');
  mdd->ShowNode(Output, p);
  Output << "\n    transtions (rows): ";
  Output.Put(mxd, 5);
  Output.Put(' ');
  mdd->ShowNode(Output, mxd);
  Output << "\n\n";
  Output.flush();
#endif
  int k = mdd->NodeLevel(p);
  s = mdd->TempNode(k, sizes[k]);
  bool snonzero = false;

  int rowsize;

  if (mdd->NodeLevel(mxd) < k) {
    // Identity node in mxd
    if (mdd->isNodeSparse(p)) {
      for (int z=mdd->nnzOf(p)-1; z>=0; z--) {
        int d = RecFire(mdd->Data(p, 2*z+1), mxd);
	if (d) {
	  snonzero = true;
	  mdd->SetArc(s, mdd->Data(p, 2*z), d);
        }
      } // for z
    } else {
      for (int i=mdd->SizeOf(p)-1; i>=0; i--) {
        int d = RecFire(mdd->Data(p, i), mxd);
	if (d) {
	  snonzero = true;
	  mdd->SetArc(s, i, d);
        }
      } // for i
    } // if p sparse
  } else {
    // Ordinary mxd node
    if (mdd->isNodeSparse(mxd)) {
      if (mdd->isNodeSparse(p)) {
	// sparse-sparse
	int pz = mdd->nnzOf(p)-1;
	int rz = mdd->nnzOf(mxd)-1;
	while (rz>=0 && pz>=0) {
	  if (mdd->Data(p, 2*pz) == mdd->Data(mxd, 2*rz)) {
	    // match, fire
	    if (FireRow(s, mdd->Data(p, 2*pz+1), mdd->Data(mxd, 2*rz+1))) 
		snonzero = true;
	    rz--;	
  	    pz--;
	    continue;
          }
	  if (mdd->Data(p, 2*pz) > mdd->Data(mxd, 2*rz)) pz--;
	  else rz--;
        }
	// done sparse-sparse
      } else { 
	// sparse-full
        int psize = mdd->SizeOf(p);
        for (int z=0; z<mdd->nnzOf(mxd); z++) {
          int i = mdd->Data(mxd, 2*z);
	  if (i>=psize) break;
	  if (FireRow(s, mdd->Data(p, i), mdd->Data(mxd, 2*z+1))) 
		snonzero = true;
        }
      }
    } else {
      rowsize = mdd->SizeOf(mxd);
      if (mdd->isNodeSparse(p)) {
	// full-sparse
        for (int z = 0; z<mdd->nnzOf(p); z++) {
  	  int i = mdd->Data(p, 2*z);
	  if (i>=rowsize) break;
	  if (FireRow(s, mdd->Data(p, 2*z+1), mdd->Data(mxd, i))) 
		snonzero = true;
        } 
      } else { 
	// full-full
	for (int i = MIN(rowsize, mdd->SizeOf(p))-1; i>=0; i--) {
	  if (FireRow(s, mdd->Data(p, i), mdd->Data(mxd, i))) 
		snonzero = true;
        } 
      }
    } // sparse rows 
  } // if skipped level
#ifdef FIRE_TRACE
  Output << "Finished RecFire node: ";
  Output.Put(p, 5);
  Output.Put(' ');
  mdd->ShowNode(Output, p);
  Output << "\n    transtions (rows): ";
  Output.Put(mxd, 5);
  Output.Put(' ');
  mdd->ShowNode(Output, mxd);
  Output << "\n               Result: ";
  Output.Put(s, 5);
  Output.Put(' ');
  mdd->ShowNode(Output, s);
  Output << "\n\n";
  Output.flush();
#endif

  if (snonzero && roots[k]) Saturate(s);
  s = mdd->Reduce(s);
  fire_cache->Add(p, mxd, s);
  return s;
}

