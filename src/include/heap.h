
// $Id$

#ifndef HEAP_H
#define HEAP_H

const int MAX_HEAP_ADD = 1024;


// ==================================================================
// ||                                                              ||
// ||                     HeapOfPointers class                     ||
// ||                                                              ||
// ==================================================================


/**  Heap template class, as in HeapSort.
     The heap stores pointers to objects.
     To use, there must be a function
        int Compare(DATA *a, DATA *b) that works like strcmp.
     Note: the heap keeps the LARGEST element at the root,
     so that the sorted array is in INCREASING order.

     Heap method "show" requires a similar method for the stored objects.
*/

template <class DATA>
class HeapOfPointers {
  DATA **data;  
  long last;
  long size;
  bool sorted;
protected:
  bool Resize(long newsize) {
    if (newsize == size) return true;
    DATA ** foo = (DATA**) realloc(data, newsize*sizeof(void*));
    if (newsize && (0==foo)) return false;
    data = foo;
    size = newsize;
    return true;
  }
public:
  /// Constructor.  Initially empty and zero size.
  HeapOfPointers() {
    data = 0;
    size = 0;
    last = 0;
    sorted = false;
  }

  /// Constructor.  Specify the initial array size.
  HeapOfPointers(long inits) {
    data = 0;
    size = 0;
    Resize(inits);
    last = 0;
    sorted = false;
  }
  /// Constructor, give initial unsorted array.
  HeapOfPointers(DATA **a, long sz) {
    data = a;
    size = sz;
    sorted = false;
    // make this array a proper heap
    last = 0;
    while (last<size) {
      UpHeap(last);
      last++; 
    } 
  }
  ~HeapOfPointers() {
    Resize(0);
  }
  void Clear(long inits) {
    last = 0;
    sorted = false;
    if (inits>size) Resize(inits);
  }
  inline long Length() { return last; }
  /// Returns true on success, false on out of memory error.
  bool Insert(DATA* a) {
    DCASSERT(!sorted);
    if (last>=size) {
      long newsize = MIN(2*size, size+MAX_HEAP_ADD);
      if (newsize < 1)  newsize = 16;
      if (!Resize(newsize))
      return false;
    }
    data[last] = a;
    UpHeap(last);
    last++;
    return true;
  }
  void Remove(DATA* &a) {
    DCASSERT(!sorted);
    Swap(0, last-1);
    DownHeap(last-1);
    last--;
    a = data[last]; 
  }
  void Sort();
  DATA** MakeArray() {
    DCASSERT(sorted);
    Resize(last);
    DATA** ret = data;
    size = last = 0;
    data = 0;
    sorted = false;
    return ret;
  }
  // for debugging
  void show(OutputStream &s) const;
protected:
  inline void Swap(long i, long j) {
    if (i!=j) {
      DATA *t = data[i];
      data[i] = data[j];
      data[j] = t;
    }
  }
  void UpHeap(long n);
  void DownHeap(long n);
};

template <class DATA>
void HeapOfPointers<DATA>::Sort()
{
  DCASSERT(!sorted);
  long n;
  for (n=last-1; n>0; n--) {
    Swap(0, n);
    DownHeap(n);
  }
  sorted = true;
}

template <class DATA>
void HeapOfPointers<DATA>::UpHeap(long n)
{
  while (n) {
    long parent = (n-1)/2;
    int cmp = Compare(data[n], data[parent]);
    if (cmp<=0) return; // child <= parent, done
    Swap(n, parent);
    n = parent;
  }
}

template <class DATA>
void HeapOfPointers<DATA>::DownHeap(long n)
{
  long x=0;
  while (1) {
    long a = 2*x+1;  // left child
    long b = a+1;  // right child
    if (a>=n) return;  // no children
    if (b>=n) {
      // this node has one left child (a leaf), and no right child.
      // check the child and swap if necessary, then we're done
      int cmp = Compare(data[a], data[x]);
      if (cmp>0) Swap(a,x); // a > x
      return;
    }
    bool xbeatsa = (Compare(data[x], data[a]) >= 0);
    bool xbeatsb = (Compare(data[x], data[b]) >= 0);
    if (xbeatsa && xbeatsb) return;  // already a heap
    if (xbeatsa) { // b is the new parent
      Swap(b,x);
      x = b; 
      continue;
    }
    if (xbeatsb) { // a is the new parent
      Swap(a,x);
      x = a;
      continue;
    }
    // Still here?  we must have a>x and b>x; 
    // New parent is max of a and b.
    if (Compare(data[a], data[b]) >= 0) {
      Swap(a,x); // a is new parent
      x = a;
    } else {
      Swap(b,x); // b is new parent
      x = b;
    }
  } // while
}

template <class DATA>
void HeapOfPointers<DATA>::show(OutputStream &s) const
{
  s << "Heap, by node:\n";
  long i;
  for (i=0; i<last; i++) {
    s << "Node " << i << ":\n";
    s << "\tobject: " << data[i] << "\n";
    long lc = 2*i+1;
    long rc = lc+1;
    if (lc<last)
      s << "\t  left: " << data[lc] << "\n";
    if (rc<last)
      s << "\t right: " << data[rc] << "\n";
    s.flush(); 
  }
  s << "End of heap\n";
  s.flush();
}



// ==================================================================
// ||                                                              ||
// ||              HeapSort functions, array of items              ||
// ||                                                              ||
// ==================================================================

/** Standard "UpHeap" procedure for an array of items.
    The array itself is modified.
    PRE: items 0 through n-1 already form a heap.
    POST: items 0 through n form a heap.
*/
template <class DATA>
void UpHeap(DATA* D, long n)
{
  while (n) {
    long parent = (n-1)/2;
    if (D[n] <= D[parent])
      return; // child <= parent, done
    SWAP(D[n], D[parent]);
    n = parent;
  }
}

/** Standard "DownHeap" procedure for an array of items.
    The array itself is modified.
    PRE: items 1 through n-1 already form a heap.
    POST: items 0 through n-1 form a heap.
*/
template <class DATA>
void DownHeap(DATA* D, long n)
{
  long x=0;
  while (1) {
    long a = 2*x+1;  // left child
    long b = a+1;  // right child
    if (a>=n) return;  // no children
    if (b>=n) {
      // this node has one left child (a leaf), and no right child.
      // check the child and swap if necessary, then we're done
      if (D[a] > D[x])
          SWAP(D[a], D[x]);  // a > x
      return;
    }
    bool xbeatsa = D[x] >= D[a];
    bool xbeatsb = D[x] >= D[b];
    if (xbeatsa && xbeatsb) return;  // already a heap
    if (xbeatsa) { // b is the new parent
      SWAP(D[x], D[b]);
      x = b; 
      continue;
    }
    if (xbeatsb) { // a is the new parent
      SWAP(D[x], D[a]);
      x = a;
      continue;
    }
    // Still here?  we must have a>x and b>x; 
    // New parent is max of a and b.
    if (D[a] >= D[b]) {
      SWAP(D[x], D[a]); // a is new parent
      x = a;
    } else {
      SWAP(D[x], D[b]); // b is new parent
      x = b;
    }
  } // while
}

/** Heapsort for an array of items.
    The array itself is modified.
    On return, data[i] gives the ith smallest item.
*/
template <class DATA>
void HeapSort(DATA* data, long N)
{
  long i;
  for (i=1; i<N; i++) {
    UpHeap(data, i);
  }
  for (i--; i>0; i--) {
    SWAP(data[0], data[i]);
    DownHeap(data, i);
  }
}

// ==================================================================
// ||                                                              ||
// ||     HeapSort functions,  array of items with permutation     ||
// ||                                                              ||
// ==================================================================

/** Standard "UpHeap" procedure for an array of items.
    The array itself is not modified, we use a permutation array instead.
    PRE: items 0 through n-1 already form a heap.
    POST: items 0 through n form a heap.
*/
template <class DATA>
void UpHeap(const DATA* D, long* perm, long n)
{
  while (n) {
    long parent = (n-1)/2;
    if (D[perm[n]] <= D[perm[parent]])
      return; // child <= parent, done
    SWAP(perm[n], perm[parent]);
    n = parent;
  }
}

/** Standard "DownHeap" procedure for an array of items.
    The array itself is not modified, we use a permutation array instead.
    PRE: items 1 through n-1 already form a heap.
    POST: items 0 through n-1 form a heap.
*/
template <class DATA>
void DownHeap(const DATA* D, long* perm, long n)
{
  long x=0;
  while (1) {
    long a = 2*x+1;  // left child
    long b = a+1;  // right child
    if (a>=n) return;  // no children
    if (b>=n) {
      // this node has one left child (a leaf), and no right child.
      // check the child and swap if necessary, then we're done
      if (D[perm[a]] > D[perm[x]])
          SWAP(perm[a], perm[x]);  // a > x
      return;
    }
    bool xbeatsa = D[perm[x]] >= D[perm[a]];
    bool xbeatsb = D[perm[x]] >= D[perm[b]];
    if (xbeatsa && xbeatsb) return;  // already a heap
    if (xbeatsa) { // b is the new parent
      SWAP(perm[x], perm[b]);
      x = b; 
      continue;
    }
    if (xbeatsb) { // a is the new parent
      SWAP(perm[x], perm[a]);
      x = a;
      continue;
    }
    // Still here?  we must have a>x and b>x; 
    // New parent is max of a and b.
    if (D[perm[a]] >= D[perm[b]]) {
      SWAP(perm[x], perm[a]); // a is new parent
      x = a;
    } else {
      SWAP(perm[x], perm[b]); // b is new parent
      x = b;
    }
  } // while
}

/** Heapsort for an array of items.
    The array itself is not modified, we use a permutation array instead.
    On return, perm[i] gives the index of the ith smallest item in D.
*/
template <class DATA>
void HeapSort(const DATA* data, long* perm, long N)
{
  long i;
  for (i=0; i<N; i++) perm[i] = i;
  for (i=1; i<N; i++) {
    UpHeap(data, perm, i);
  }
  for (i--; i>0; i--) {
    SWAP(perm[0], perm[i]);
    DownHeap(data, perm, i);
  }
}

// ==================================================================
// ||                                                              ||
// ||            HeapSort functions, general collection            ||
// ||                                                              ||
// ==================================================================

/** Standard "UpHeap" procedure for an arbitrary collection.
    PRE: items 0 through n-1 already form a heap.
    POST: items 0 through n form a heap.
    The collection class must provide functions:
      Compare(long i, long j)
      Swap(long i, long j)
    where \a i and \a j are "item numbers".
*/
template <class COLL>
void UpHeapAbstract(COLL* data, long n)
{
  while (n) {
    long parent = (n-1)/2;
    int cmp = data->Compare(n, parent);
    if (cmp<=0) return; // child <= parent, done
    data->Swap(n, parent);
    n = parent;
  }
}

/** Standard "DownHeap" procedure for an arbitrary collection.
    PRE: items 1 through n-1 already form a heap.
    POST: items 0 through n-1 form a heap.
    The collection class must provide functions:
      Compare(long i, long j)
      Swap(long i, long j)
    where \a i and \a j are "item numbers".
*/
template <class COLL>
void DownHeapAbstract(COLL* data, long n)
{
  long x=0;
  while (1) {
    long a = 2*x+1;  // left child
    long b = a+1;  // right child
    if (a>=n) return;  // no children
    if (b>=n) {
      // this node has one left child (a leaf), and no right child.
      // check the child and swap if necessary, then we're done
      int cmp = data->Compare(a, x);
      if (cmp>0) data->Swap(a,x); // a > x
      return;
    }
    bool xbeatsa = (data->Compare(x, a) >= 0);
    bool xbeatsb = (data->Compare(x, b) >= 0);
    if (xbeatsa && xbeatsb) return;  // already a heap
    if (xbeatsa) { // b is the new parent
      data->Swap(b,x);
      x = b; 
      continue;
    }
    if (xbeatsb) { // a is the new parent
      data->Swap(a,x);
      x = a;
      continue;
    }
    // Still here?  we must have a>x and b>x; 
    // New parent is max of a and b.
    if (data->Compare(a, b) >= 0) {
      data->Swap(a,x); // a is new parent
      x = a;
    } else {
      data->Swap(b,x); // b is new parent
      x = b;
    }
  } // while
}

/** Heapsort for an arbitrary collection.
    The collection class must provide functions:
      Compare(long i, long j)
      Swap(long i, long j)
    where \a i and \a j are "item numbers".
*/
template <class COLL>
void HeapSortAbstract(COLL* data, long N)
{
  long i;
  for (i=1; i<N; i++) {
    UpHeapAbstract(data, i);
  }
  for (i--; i>0; i--) {
    data->Swap(0, i);
    DownHeapAbstract(data, i);
  }
}

#endif

