
// $Id$

/*
    New array-based stack templates.
*/

#ifndef STACK_H
#define STACK_H

#include <stdlib.h>

template <class DATA>
class Stack {
  DATA *data;
  int size;
  int top;
  int maxsize;  
protected:
  void Enlarge(int newsize) {
    if (size == maxsize) {
      // better failure here...
      cerr << "Stack overflow\n";
      exit(0);
    }
    DATA *foo = realloc(data, newsize*sizeof(DATA));
    if (NULL==foo) {
      // handle failure better here...
      cerr << "Memory overflow on stack resize\n";
      exit(0);
    }
    data = foo;
    size = newsize;
  }
public:
  Stack(int currentsize, int msize=1000000000) {
    // 1 billion max by default is effectively infinite
    size = currentsize;
    top = 0;
    maxsize = msize;
    data = malloc(currentsize * sizeof(DATA));
  }
  ~Stack() { free(data); }
  inline bool Empty() const { return 0==top; }
  inline int NumEntries() const { return top; }
  inline void Push(const DATA &x) {
    if (top>=size) Enlarge(MIN(2*size, maxsize));
    data[top] = x;
    top++;
  }
  inline DATA Pop() {
    DCASSERT(top);
    return data[top--];
  }
  inline DATA Top() const {
    DCASSERT(top);
    return data[top-1];
  }
  // Not so efficient, but sometimes necessary...
  bool Contains(const DATA &key) const {
    for (int i=0; i<top; i++)
      if (data[i] == key) return true;
    return false;
  }
};

typedef Stack<void*> PtrStack;

#endif

