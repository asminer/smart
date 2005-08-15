
// $Id$

/*
    New array-based queue templates.
*/

#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include "../Base/errors.h"

template <class DATA>
class Queue {
  DATA *data;
  int* next;
  int size;
  int head;
  int tail;
  int lastused;
  int unused;
protected:
  void Enlarge(int newsize) {
    if (size == maxsize) {
      Internal.Start(__FILE__, __LINE__);
      Internal << "Queue overflow";
      Internal.Stop();
    }
    data = (DATA *) realloc(data, newsize*sizeof(DATA));
    if (NULL==data) OutOfMemoryError("queue resize");
    next = (int *) realloc(next, newsize*sizeof(int));
    if (NULL==next) OutOfMemoryError("queue resize");
    size = newsize;
  }
public:
  Queue(int currentsize) {
    // 1 billion max by default is effectively infinite
    size = 0;
    data = NULL;
    next = NULL;
    head = tail = unused = -1;
    numentries = lastused = 0;
    maxsize = msize;
    Enlarge(currentsize);
  }
  ~Queue() { free(data); free(next); }
  inline bool Empty() const { return (head<0); }
  inline int AllocEntries() const { return size; }
  inline void Clear() { size = 0; head = tail = unused = -1; }
  inline void Push(const DATA &x) {
    int a;
    if (unused>=0) {
      a = unused;
      unused = next[a];
    } else {
      a = ++lastused;
      if (lastused >= size) Enlarge(MIN(2*size, size+1024)); 
    }
    data[a] = x;
    next[a] = -1;
    if (tail>=0) next[tail] = a;
    else head = a; 
    tail = a;
    numentries++;
  }
  inline DATA Pop() {
    DCASSERT(head>=0);
    int a = head;
    head = next[a];
    if (head<0) tail = -1;
    if (a==lastused) {
      lastused--;
    } else {
      next[a] = unused;
      unused = a;
    }
    return data[a];
  }
  inline DATA Top() const {
    DCASSERT(head>=0);
    return data[head];
  }
};

#endif

