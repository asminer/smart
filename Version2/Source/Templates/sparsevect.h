
// $Id$

/*  
    Class for sparse vectors
*/

#ifndef SPARSEVECT_H
#define SPARSEVECT_H

#include "../Base/errors.h"
#include "heap.h"  // for sorting

class sparse_bitvector {
public:
  /// Number of nonzero entries
  int nonzeroes;
  /// Vector of nonzero indices
  int* index; 
  /// Size of index vector (some elements unused)
  int size;
  /// Is the index vector sorted
  bool isSorted;
protected:
  void Resize(int newsize) {
    int *foo = (int *) realloc(index, newsize*sizeof(int));
    if (newsize && (NULL==foo)) OutOfMemoryError("Sparse vector resize");
    index = foo;
    size = newsize;
  }
public:
  sparse_bitvector(int inits) {
    index = NULL;
    Resize(MAX(inits, 1));
    nonzeroes = 0;
    isSorted = true;
  }
  ~sparse_bitvector() { 
    Resize(0);
  }
  inline void Clear() { nonzeroes = 0; isSorted = true; }
  inline int NumNonzeroes() const { return nonzeroes; }
  int BinarySearchIndex(int e) const {
    DCASSERT(isSorted);
    int high = nonzeroes;
    int low = 0;
    while (low < high) {
      int mid = (high+low)/2;
      if (e == index[mid]) return mid;
      if (e < index[mid]) high = mid;
      else low = mid+1;
    }
    return -1;
  }
  int LinearSearchIndex(int e) const {
    int low;  
    for (low=nonzeroes-1; low>=0; low--) 
      if (e == index[low]) return low;
    return -1;
  }
  inline void Append(int a) {
    while (nonzeroes >= size) Resize(size + MAX(size/2, 1));
    index[nonzeroes] = a;
    nonzeroes++;
    isSorted = false;
  }
  inline void SortedAppend(int a) {
    DCASSERT(isSorted);
    while (nonzeroes >= size) Resize(size + MAX(size/2, 1));
    int i = nonzeroes;
    while (i) {
      if (a < index[i-1]) 
	index[i] = index[i-1];
      else {
	break;
      }
      i--;
    }
    index[i] = a; 
    nonzeroes++;
  }
  void Sort() {
    if (isSorted) return;
    HeapOfObjects <int> foo(index, nonzeroes); 
    foo.Sort();
    index = foo.MakeArray();
    size = nonzeroes;
    isSorted = true;
  }
};


template <class DATA>
class sparse_vector : public sparse_bitvector {
public:
  /// Value of each nonzero entry
  DATA* value;
protected:
  void Resize(int newsize) {
    sparse_bitvector::Resize(newsize);
    DATA *bar = (DATA *) realloc(value, newsize*sizeof(DATA));
    if (newsize && (NULL==bar)) OutOfMemoryError("Sparse vector resize");
    value = bar;
  }
public:
  sparse_vector(int inits) : sparse_bitvector(0) {
    index = NULL;
    value = NULL;
    Resize(MAX(inits, 1));
    nonzeroes = 0;
    isSorted = true;
  }
  ~sparse_vector() { 
    Resize(0);
  }
  inline void Append(int i, DATA v) {
    while (nonzeroes >= size) Resize(size + MAX(size/2, 1));
    index[nonzeroes] = i;
    value[nonzeroes] = v;
    nonzeroes++;
    isSorted = false;
  }
  inline void SortedAppend(int i, DATA v) {
    DCASSERT(isSorted);
    while (nonzeroes >= size) Resize(size + MAX(size/2, 1));
    int n = nonzeroes;
    while (n) {
      if (i < index[n-1]) {
        index[n] = index[n-1];
        value[n] = value[n-1];
      } else {
	break;
      }
      n--;
    }
    index[n] = i; 
    value[n] = v;
    nonzeroes++;
  }
  void Sort() {
    if (isSorted) return;
    int i;
    for (i=1; i<nonzeroes; i++) UpHeap(i);
    for (i=nonzeroes-1; i>0; i--) {
      SWAP(index[0], index[i]);
      SWAP(value[0], value[i]);
      DownHeap(i);
    }
    isSorted = true;
  }
protected:
  void UpHeap(int n) {
    while (n) {
      int parent = n/2;
      if (index[n] < index[parent]) return;
      SWAP(index[n], index[parent]);
      SWAP(value[n], value[parent]);
      n = parent;
    }
  };
  void DownHeap(int n);
};

template <class DATA>
void sparse_vector <DATA> :: DownHeap(int n)
{
  int x=0;
  while (1) {
    int a = 2*x+1;  // left child
    int b = a+1;  // right child
    if (a>=n) return;  // no children
    if (b>=n) {
        // this node has one left child (a leaf), and no right child.
        // check the child and swap if necessary, then we're done
	if (index[a] > index[x]) {
	  SWAP(index[a], index[x]);
	  SWAP(value[a], value[x]);
        }
        return;
    }
    bool xbeatsa = (index[x] > index[a]);
    bool xbeatsb = (index[x] > index[b]);
    if (xbeatsa && xbeatsb) return;  // already a heap
    if (xbeatsa) { 
	// b is the new parent
	SWAP(index[b], index[x]);
	SWAP(value[b], value[x]);
        x = b; 
        continue;
    }
    if (xbeatsb) { 
   	// a is the new parent
	SWAP(index[a], index[x]);
	SWAP(value[a], value[x]);
        x = a;
        continue;
    }
    // Still here?  we must have a>x and b>x; 
    // New parent is max of a and b.
    if (index[a] >= index[b]) {
        // a is new parent
	SWAP(index[a], index[x]);
	SWAP(value[a], value[x]);
        x = a;
    } else {
        // b is new parent
	SWAP(index[b], index[x]);
	SWAP(value[b], value[x]);
        x = b;
    }
  } // while
}

#endif

