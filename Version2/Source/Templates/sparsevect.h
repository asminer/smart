
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
    DCASSERT(0);
  }
};

#endif

