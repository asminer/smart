
// $Id$

/**  Heap template class, as in HeapSort.
     To use, there must be a function
        int Compare(DATA *a, DATA *b) that works like strcmp.
*/

#ifndef HEAP_H
#define HEAP_H

template <class DATA>
class Heap {
  DATA **data;  
  int last;
  int size;
  bool sorted;
protected:
  void Resize(int newsize) {
    DATA ** foo = (DATA**) realloc(data, newsize*sizeof(void*));
    if (newsize && (NULL==foo)) {
      Internal.Start(__FILE__, __LINE__);
      Internal << "Memory overflow on Heap resize\n";
      Internal.Stop();
    }
    data = foo;
    size = newsize;
  }
public:
  /// Constructor.  Specify the initial array size.
  Heap(int inits) {
    data = NULL;
    Resize(inits);
    last = 0;
    sorted = false;
  }
  ~Heap() {
    Resize(0);
  }
  inline int Length() { return last; }
  void Insert(DATA *a) {
    DCASSERT(!sorted);
    if (last>=size) Resize(2*size);
    data[last] = a;
    UpHeap(last);
    last++;
  }
  /// Assumes all elements were added via "insert"
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
void Heap<DATA>::Sort()
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
void Heap<DATA>::UpHeap(int n)
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
void Heap<DATA>::DownHeap(int n)
{
  int x=0;
  while (1) {
    int a = 2*x;  // left child
    int b = a+1;  // right child
    if (a>=n) return;  // no children
    if (b>=last) {
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

