
// $Id$

#include "mddops_malloc.h"

// #define UNION_TRACE
// #define FIRE_TRACE
// #define SATURATE_TRACE

// ************************************************************
// *                   binary_cache methods                   *
// ************************************************************

binary_cache::binary_cache(mdd_node_manager* m)
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

operations::operations(mdd_node_manager* m)
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
  DCASSERT(mdd->LevelOf(a) == mdd->LevelOf(b));
  int k = mdd->LevelOf(a);
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
  if (mdd->isSparse(a)) {
    // a is sparse
    const int* aindex = mdd->IndexesOf(a);
    const int* adown = mdd->SparseDownOf(a);
    if (mdd->isSparse(b)) {
      // b is sparse
      const int* bindex = mdd->IndexesOf(b);
      const int* bdown = mdd->SparseDownOf(b);
      csz = MAX(aindex[mdd->nnzOf(a)-1], bindex[mdd->nnzOf(b)-1]) + 1;
      c = mdd->TempNode(k, csz);
      const int* cdown = mdd->FullDownOf(c);
      // Copy nonzeroes of a to c
      for (int z = mdd->nnzOf(a)-1; z>=0; z--) {
        mdd->Link(adown[z]);
        mdd->SetArc(c, aindex[z], adown[z]);
      }
      // Union nonzeroes of b
      for (int z = mdd->nnzOf(b)-1; z>=0; z--) {
        int i = bindex[z];
        mdd->SetArc(c, i, Union(cdown[i], bdown[z]));  
      }
      // done sparse-sparse
    } else {
      // b is full
      const int* bdown = mdd->FullDownOf(b);
      csz = MAX(mdd->SizeOf(b), 1+aindex[mdd->nnzOf(a)-1]); 
      c = mdd->TempNode(k, csz);
      const int* cdown = mdd->FullDownOf(c);
      // Copy b to c
      for (int i = mdd->SizeOf(b)-1; i>=0; i--) {
        mdd->Link(bdown[i]);
        mdd->SetArc(c, i, bdown[i]);
      }
      // Union nonzeroes of a
      for (int z = mdd->nnzOf(a)-1; z>=0; z--) {
        int i = aindex[z];
        mdd->SetArc(c, i, Union(cdown[i], adown[z]));  
      }
      // done sparse-full
    }
  } else {
    // a is full
    const int* adown = mdd->FullDownOf(a);
    if (mdd->isSparse(b)) {
      // b is sparse
      const int* bindex = mdd->IndexesOf(b);
      const int* bdown = mdd->SparseDownOf(b);
      csz = MAX(mdd->SizeOf(a), 1+bindex[mdd->nnzOf(b)-1]); 
      c = mdd->TempNode(k, csz);
      const int* cdown = mdd->FullDownOf(c);
      // Copy a to c
      for (int i = mdd->SizeOf(a)-1; i>=0; i--) {
        mdd->Link(adown[i]);
        mdd->SetArc(c, i, adown[i]);
      }
      // Union nonzeroes of b
      for (int z = mdd->nnzOf(b)-1; z>=0; z--) {
        int i = bindex[z];
        mdd->SetArc(c, i, Union(cdown[i], bdown[z]));  
      }
      // done full-sparse
    } else {
      // b is full
      const int* bdown = mdd->FullDownOf(b);
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
  if (mdd->isSparse(a)) {
    const int* adown = mdd->SparseDownOf(a);
    for (int i = mdd->nnzOf(a)-1; i>=0; i--) {
      counts[a] += Count(adown[i]);
    }
  } else {
    const int* adown = mdd->FullDownOf(a);
    for (int i = mdd->SizeOf(a)-1; i>=0; i--) {
      counts[a] += Count(adown[i]);
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
  int k = mdd->LevelOf(init);
  DCASSERT(mdd->SizeOf(init)==sizes[k]);
  const int* down = mdd->FullDownOf(init);
  for (int i=mdd->SizeOf(init)-1; i>=0; i--) 
    if (down[i]) TopSaturate(down[i]);
  if (roots[k]) Saturate(init);
}

void operations::Saturate(int init)
{
  int k = mdd->LevelOf(init);
  DCASSERT(k>0);  
  DCASSERT(roots[k]);

#ifdef UNION_TRACE
  Output << "Saturating " << init << " ";
  mdd->ShowNode(Output, init);
  Output << "\n\n";
  Output.flush();
#endif
  const int* down = mdd->FullDownOf(init);

  int rowlist = roots[k];
  bool rowsparse = mdd->isSparse(rowlist);
  const int* rdown;
  const int* rindex;
  int rowsize;
  Lset[k]->SetMax(sizes[k]);

  if (rowsparse) { 
    rowsize = mdd->nnzOf(rowlist);
    rindex = mdd->IndexesOf(rowlist);
    rdown = mdd->SparseDownOf(rowlist);
    for (int z=rowsize-1; z>=0; z--) {
      CHECK_RANGE(0, rindex[z], mdd->SizeOf(init));
      if (down[rindex[z]]) Lset[k]->AddElement(rindex[z]);
    } // for z
  } else {
    rowsize = mdd->SizeOf(rowlist);
#ifdef DEVELOPMENT_CODE
    rindex = NULL;
#endif
    rdown = mdd->FullDownOf(rowlist);
    DCASSERT(rowsize <= mdd->SizeOf(init));
    for (int i=rowsize-1; i>=0; i--) {
      if (down[i] && rdown[i]) Lset[k]->AddElement(i);
    } // for i
  } // if rowsparse

  while (Lset[k]->Card()) {
    int collist;
    bool colsparse;
    int colsize;
    int i = Lset[k]->RemoveElement();
    if (rowsparse) {
      int z;
      for (z=rowsize-1; z>=0; z--) {
        if (rindex[z] == i) break;
        if (rindex[z] < i) z = 0; // will never be found
      }
      if (z<0) continue;  // not found
      collist = rdown[z];
      colsparse = mdd->isSparse(collist);
    } else {
      if (i>=rowsize) continue;
      collist = rdown[i];
      if (0==collist) continue;
      colsparse = mdd->isSparse(collist);
    } // if rowsparse

    if (colsparse) {
      const int* cindex = mdd->IndexesOf(collist);
      const int* cdown = mdd->SparseDownOf(collist);
      // scan through sparse column
      for (int z=mdd->nnzOf(collist)-1; z>=0; z--) {
        int j = cindex[z];
        int f = RecFire(down[i], cdown[z]);
        if (f) {
	  int u = Union(f, down[j]);
   	  mdd->Unlink(f);
	  if (u!=down[j]) {
	    mdd->SetArc(init, j, u);
	    Lset[k]->AddElement(j);
          } // if u
        } // f 
      } // for z
    } else {
      const int* cdown = mdd->FullDownOf(collist);
      // scan through full column
      for (int j=mdd->SizeOf(collist)-1; j>=0; j--) {
        int f = RecFire(down[i], cdown[j]);
        if (f && f!=down[j]) {
	  int u = Union(f, down[j]);
   	  mdd->Unlink(f);
	  if (u!=down[j]) {
	    mdd->SetArc(init, j, u);
	    Lset[k]->AddElement(j);
          } else {
	    mdd->Unlink(down[j]);
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
  const int* sdown = mdd->FullDownOf(s);
  if (mdd->isSparse(row)) {
    const int* rindex = mdd->IndexesOf(row);
    const int* rdown = mdd->SparseDownOf(row);
    for (int z = mdd->nnzOf(row)-1; z>=0; z--) {
      int j = rindex[z];
      int f = RecFire(pd, rdown[z]);
      int u = Union(sdown[j], f);
      mdd->Unlink(f);
      mdd->SetArc(s, j, u);
      if (u) nonzero = true;
    }
  } else {
    const int* rdown = mdd->FullDownOf(row);
    for (int j = mdd->SizeOf(row)-1; j>=0; j--) {
      int f = RecFire(pd, rdown[j]);
      int u = Union(sdown[j], f);
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
  int k = mdd->LevelOf(p);
  s = mdd->TempNode(k, sizes[k]);
  const int* sdown = mdd->FullDownOf(s);
  bool snonzero = false;

  if (mdd->LevelOf(mxd) < k) {
    // Identity node in mxd
    if (mdd->isSparse(p)) {
      const int* pindex = mdd->IndexesOf(p);
      const int* pdown = mdd->SparseDownOf(p);
      for (int z=mdd->nnzOf(p)-1; z>=0; z--) {
        int d = RecFire(pdown[z], mxd);
	if (d) {
	  snonzero = true;
	  mdd->SetArc(s, pindex[z], d);
        }
      } // for z
    } else {
      const int* pdown = mdd->FullDownOf(p);
      for (int i=mdd->SizeOf(p)-1; i>=0; i--) {
        int d = RecFire(pdown[i], mxd);
	if (d) {
	  snonzero = true;
	  mdd->SetArc(s, i, d);
        }
      } // for i
    } // if p sparse
  } else {
    // Ordinary mxd node
    if (mdd->isSparse(mxd)) {
      const int* rindex = mdd->IndexesOf(mxd);
      const int* rdown = mdd->SparseDownOf(mxd);
      if (mdd->isSparse(p)) {
	// sparse-sparse
        const int* pindex = mdd->IndexesOf(p);
        const int* pdown = mdd->SparseDownOf(p);
	int pz = mdd->nnzOf(p)-1;
	int rz = mdd->nnzOf(mxd)-1;
	while (rz>=0 && pz>=0) {
	  if (pindex[pz] == rindex[rz]) {
	    // match, fire
	    if (FireRow(s, pdown[pz], rdown[rz])) 
		snonzero = true;
	    rz--;	
  	    pz--;
	    continue;
          }
	  if (pindex[pz] > rindex[rz]) pz--;
	  else rz--;
        }
	// done sparse-sparse
      } else { 
	// sparse-full
        const int* pdown = mdd->FullDownOf(p);
        int psize = mdd->SizeOf(p);
        for (int z=0; z<mdd->nnzOf(mxd); z++) {
	  if (rindex[z]>=psize) break;
	  if (FireRow(s, pdown[rindex[z]], rdown[z])) 
		snonzero = true;
        }
      }
    } else {
      int rowsize = mdd->SizeOf(mxd);
      const int* rdown = mdd->FullDownOf(mxd);
      if (mdd->isSparse(p)) {
	// full-sparse
        const int* pindex = mdd->IndexesOf(p);
        const int* pdown = mdd->SparseDownOf(p);
        for (int z = 0; z<mdd->nnzOf(p); z++) {
	  if (pindex[z]>=rowsize) break;
	  if (FireRow(s, pdown[z], rdown[pindex[z]])) 
		snonzero = true;
        } 
      } else { 
	// full-full
        const int* pdown = mdd->FullDownOf(p);
	for (int i = MIN(rowsize, mdd->SizeOf(p))-1; i>=0; i--) {
	  if (FireRow(s, pdown[i], rdown[i])) 
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

