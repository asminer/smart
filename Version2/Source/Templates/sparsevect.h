
// $Id$

/*  
    Class for sparse vectors
*/

#ifndef SPARSEVECT_H
#define SPARSEVECT_H

#include "../Base/errors.h"

class sparse_bitvector {
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
  int BinaryFindIndex(int e) const {
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
  int LinearFindIndex(int e) const {
    int low;  
    for (low=nonzeroes-1; low>=0; low--) 
      if (e == index[low]) return low;
    return -1;
  }
  inline int GetNthNonzeroIndex(int n) const {
    DCASSERT(n<nonzeroes);
    DCASSERT(n>=0);
    return index[n];
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
    // heapsort here
    isSorted = true;
  }
};

#endif

