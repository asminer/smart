
// $Id$

#ifndef INTSET_H
#define INTSET_H

#include "../defines.h"
#include "../Base/errors.h"

#ifdef DEBUG_INTSET
#include "../Base/streams.h"
#endif

/**
    Integer set. 
    Note it behaves as a queue to remove elements.
*/
class int_set {
  int* next;
  int last;
  int size;
  int head;
  int tail;
protected:
  void EnlargeData(int sz);
public:
  int_set();
  ~int_set();

  inline void AddElement(int i) {
    DCASSERT(i>=0);
    if (i>=size) EnlargeData(i);
    if (next[i]>=0) return;
    if (tail>=0) {
      next[tail] = i;
    } else {
      // empty queue
      head = i;
    }
    next[i] = i;  // end of list marker
    tail = i;
#ifdef DEBUG_INTSET
    Output << "Added element " << i << "\n";
    Dump();
#endif
  }

#ifdef DEBUG_INTSET
  void Dump() {
    Output << "Next array: [";
    Output.PutArray(next, size);
    Output << "]\n";
    Output << "head: " << head << "\ttail: " << tail << "\n";
    Output.flush();
  }
#endif

  inline bool Contains(int i) const { 
    DCASSERT(i>=0);
    if (i>=size) return false;
    return next[i] >= 0;
  }

  inline bool isEmpty() const { return (head<0); }
  inline bool notEmpty() const { return (head>=0); }

  inline int RemoveElement() {
    DCASSERT(head>=0); 
    int a = head;
    if (next[a] != a) {
      // not the last element
      head = next[a];
    } else {
      // last element
      head = tail = -1;
    }
    next[a] = -1;
#ifdef DEBUG_INTSET
    Output << "Removed element " << a << "\n";
    Dump();
#endif
    return a;
  }
};

#endif

