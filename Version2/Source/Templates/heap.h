
// $Id$

#ifndef HEAP_H
#define HEAP_H

#include "list.h"
#include "../Base/errors.h"

// ==================================================================
// ||                                                              ||
// ||                     HeapOfObjects  class                     ||
// ||                                                              ||
// ==================================================================

/**  Heap template class, as in HeapSort.
     The heap stores objects directly.
     To use, the comparison operator > must be overloaded.
     Note: the heap keeps the LARGEST element at the root,
     so that the sorted array is in INCREASING order.
*/

template <class DATA> 
class HeapOfObjects {
  DATA *data;
  int last;
  int size;
  bool sorted;
protected:
  void Resize(int newsize) {
    if (newsize == size) return;
    DATA * foo = (DATA*) realloc(data, newsize*sizeof(DATA));
    if (newsize && (NULL==foo)) OutOfMemoryError("Heap resize");
    data = foo;
    size = newsize;
  }
public:
  /// Constructor.  Specify the initial array size.
  HeapOfObjects(int inits) {
    data = NULL;
    Resize(inits);
    last = 0;
    sorted = false;
  }
  /// Constructor, give initial unsorted array.
  HeapOfObjects(DATA *a, int sz) {
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
  ~HeapOfObjects() {
    Resize(0);
  }
  void Insert(DATA a) {
    DCASSERT(!sorted);
    if (last>=size) Resize(2*size);
    data[last] = a;
    UpHeap(last);
    last++;
  }
  void Remove(DATA &a) {
    DCASSERT(!sorted);
    Swap(0, last-1);
    DownHeap(last-1);
    last--;
    a = data[last]; 
  }
  void Sort();
  DATA* MakeArray() {
    DCASSERT(sorted);
    Resize(last);
    DATA* ret = data;
    size = last = 0;
    data = NULL;
    sorted = false;
    return ret;
  }
protected:
  inline void Swap(int i, int j) {
    if (i!=j) {
      DATA t = data[i];
      data[i] = data[j];
      data[j] = t;
    }
  }
  void UpHeap(int n);
  void DownHeap(int n);
};

template <class DATA>
void HeapOfObjects<DATA>::Sort()
{
  DCASSERT(!sorted);
  int n;
  for (n=last-1; n>0; n--) {
    Swap(0, n);
    DownHeap(n);
  }
  sorted = true;
}

template <class DATA>
void HeapOfObjects<DATA>::UpHeap(int n)
{
  while (n) {
    int parent = n/2;
    if (!(data[n] > data[parent])) return;  // child <= parent, we're a heap
    Swap(n, parent);
    n = parent;
  }
}

template <class DATA>
void HeapOfObjects<DATA>::DownHeap(int n)
{
  int x=0;
  while (1) {
    int a = 2*x+1;  // left child
    int b = a+1;  // right child
    if (a>=n) return;  // no children
    if (b>=n) {
      // this node has one left child (a leaf), and no right child.
      // check the child and swap if necessary, then we're done
      if (data[a] > data[x]) Swap(a,x);
      return;
    }
    bool xbeatsa = !(data[a] > data[x]);
    bool xbeatsb = !(data[b] > data[x]);
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
    if (data[a] > data[b]) {
      Swap(a,x); // a is new parent
      x = a;
    } else {
      Swap(b,x); // b is new parent
      x = b;
    }
  } // while
}



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
*/

template <class DATA>
class HeapOfPointers {
  DATA **data;  
  int last;
  int size;
  bool sorted;
protected:
  void Resize(int newsize) {
    if (newsize == size) return;
    DATA ** foo = (DATA**) realloc(data, newsize*sizeof(void*));
    if (newsize && (NULL==foo)) OutOfMemoryError("Heap resize");
    data = foo;
    size = newsize;
  }
public:
  /// Constructor.  Specify the initial array size.
  HeapOfPointers(int inits) {
    data = NULL;
    Resize(inits);
    last = 0;
    sorted = false;
  }
  /// Constructor, give initial unsorted array.
  HeapOfPointers(DATA **a, int sz) {
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
  inline int Length() { return last; }
  void Insert(DATA* a) {
    DCASSERT(!sorted);
    if (last>=size) Resize(2*size);
    data[last] = a;
    UpHeap(last);
    last++;
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
    data = NULL;
    sorted = false;
    return ret;
  }
  List <DATA> *MakeList() {
    DCASSERT(sorted);
    List <DATA> *foo = new List <DATA> (data, size, last);
    size = last = 0;
    data = NULL;
    sorted = false;
    return foo;
  }
protected:
  inline void Swap(int i, int j) {
    if (i!=j) {
      DATA *t = data[i];
      data[i] = data[j];
      data[j] = t;
    }
  }
  void UpHeap(int n);
  void DownHeap(int n);
};

template <class DATA>
void HeapOfPointers<DATA>::Sort()
{
  DCASSERT(!sorted);
  int n;
  for (n=last-1; n>0; n--) {
    Swap(0, n);
    DownHeap(n);
  }
  sorted = true;
}

template <class DATA>
void HeapOfPointers<DATA>::UpHeap(int n)
{
  while (n) {
    int parent = n/2;
    int cmp = Compare(data[n], data[parent]);
    if (cmp<=0) return; // child <= parent, done
    Swap(n, parent);
    n = parent;
  }
}

template <class DATA>
void HeapOfPointers<DATA>::DownHeap(int n)
{
  int x=0;
  while (1) {
    int a = 2*x+1;  // left child
    int b = a+1;  // right child
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

#endif

