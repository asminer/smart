
// $Id$

/*  
    Class for sparse vectors
*/

class sparse_bitvector {
  /// Number of nonzero entries
  int nonzeroes;
  /// Vector of nonzero indices
  int* index; 
  /// Size of index vector (some elements unused)
  int size;
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
    Resize(inits);
    nonzeroes = 0;
  }
  ~sparse_bitvector() { 
    Resize(0);
  }
  inline void Clear() { nonzeroes = 0; }
  inline int NumNonzeroes() const { return nonzeroes; }
  int Find(int e) const {
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
  inline bool Contains(int e) const { return Find(e)>=0; }
  inline int GetNthNonzeroIndex(int n) const {
    DCASSERT(n<nonzeroes);
    DCASSERT(n>=0);
    return index[n];
  }
};
