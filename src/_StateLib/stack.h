
// $Id$

/*
    New array-based stack templates.
*/

#ifndef STACK_H
#define STACK_H

#include <stdlib.h>

// #define DEBUG_STACK_OVERFLOW

template <class DATA>
class Stack {
  DATA *data;
  long size;
  long top;
  long maxsize;  
protected:
  void Enlarge(long newsize) {
    if (size == maxsize) {
#ifdef DEBUG_STACK_OVERFLOW
      fprintf(stderr, "stack already max size %ld\n", maxsize);
#endif
      throw StateLib::error(StateLib::error::StackOver);
    }
    DATA *foo = (DATA *) realloc(data, newsize*sizeof(DATA));
    if (0==foo) {
#ifdef DEBUG_STACK_OVERFLOW
      fprintf(stderr, "realloc failed on data for size %ld\n", newsize*sizeof(DATA));
#endif
      throw StateLib::error(StateLib::error::StackOver);
    }
    data = foo;
    size = newsize;
  }
public:
  Stack(long currentsize, long msize=1000000000) {
    // 1 billion max by default is effectively infinite
    size = 0;
    data = NULL;
    top = 0;
    maxsize = msize;
    Enlarge(currentsize);
  }
  ~Stack() { free(data); }
  inline bool Empty() const { return 0==top; }
  inline long NumEntries() const { return top; }
  inline long AllocEntries() const { return size; }
  inline void Clear() { top = 0; }
  inline long BytesUsed() const { return size * sizeof(DATA); }
  inline void Push(const DATA &x) {
    if (top>=size) Enlarge((2*size > maxsize) ? maxsize : 2*size);
    data[top] = x;
    top++;
  }
  inline DATA Pop() {
    top--;
    return data[top];
  }
  inline DATA Top() const {
    return data[top-1];
  }
  // Not so efficient, but sometimes necessary...
  bool Contains(const DATA &key) const {
    for (long i=0; i<top; i++)
      if (data[i] == key) return true;
    return false;
  }

  // reset the max stack size
  bool SetMaxSize(long msize) {
    maxsize = (msize>16) ? msize : 16;
#ifdef DEBUG_STACK_OVERFLOW
    fprintf(stderr, "Set stack size to %ld\n", maxsize);
#endif
    if (size <= maxsize) return true;
    DATA* foo = (DATA*) realloc(data, maxsize*sizeof(DATA));
    if (0==foo) return false;
    data = foo;
    return true;
  }
};

typedef Stack<void*> PtrStack;

#endif

