
// $Id$

/*  
    Replacement for linked lists of pointers: use arrays!
*/

#ifndef LIST_H
#define LIST_H

// for memcpy
#include <string.h>
#include "../Base/errors.h"
#include "../Base/memtrack.h"

const int MAX_LIST_ADD = 1024;


template <class DATA>
struct DataList {
  DATA* data;
  int alloc;
  int last;
  void Resize(int newsize) {
    data = (DATA*) realloc(data, newsize*sizeof(DATA));
    if (newsize && (NULL==data)) OutOfMemoryError("List resize");
    alloc = newsize;
  }
public:
  DataList(int inits) {
    ALLOC("DataList", sizeof(DataList));
    data = NULL;
    Resize(inits);
    last = 0;
  }
  ~DataList() {
    FREE("DataList", sizeof(DataList));
    Resize(0);
  }
  inline int Length() const { return last; }
  inline void Pop() { if (last) last--; }
  inline void AppendBlank() {
    if (last>=alloc) Resize(MIN(2*alloc, alloc+MAX_LIST_ADD));
    last++;
  }
  inline void Clear() { last = 0; }
  DATA* CopyArray() const {
    if (0==last) return NULL;
    DATA* c = new DATA[last]; 
    memcpy(c, data, last * sizeof(DATA));
    return c;
  }
  /** Equivalent to CopyArray followed by Clear.
      Since this trashes our copy, this should be used
      when we were going to delete the original anyway.
  */
  DATA* CopyAndClearArray() {
    if (0==last) return NULL;
    Resize(last);
    DATA* a = data;
    data = NULL;
    alloc = last = 0;
    return a;
  }
};

class PtrList : public DataList <void*> {
public:
  PtrList(int inits) : DataList <void*> (inits) { }

  inline void* VItem(int n) const { 
    CHECK_RANGE(0, n, last);
    DCASSERT(data);
    return data[n]; 
  }

  inline void SetItem(int n, void *v) { 
    CHECK_RANGE(0, n, last);
    DCASSERT(data);
    data[n] = v;
  }

  void VInsertAt(int n, void* x) {
    CHECK_RANGE(0, n, 1+last);
    DCASSERT(data);
    if (last >= alloc) Resize(MIN(2*alloc, alloc+MAX_LIST_ADD));
    for (int i = last; i>n; i--) {
      data[i] = data[i-1];
    }
    data[n] = x;
    last++;
  }

  void VAppend(void* x) {
    if (last>=alloc) Resize(MIN(2*alloc, alloc+MAX_LIST_ADD));
    data[last] = x;
    last++;
  }

  void VAppend(PtrList *x) {
    if (x) {
      while (last + x->last >= alloc) alloc*=2;
      Resize(alloc);
      // risky....
      memcpy(data+last, x->data, x->last * sizeof(void*));
      last += x->last;
      delete x;
    }
  }

};

template <class DATA> 
class List : public PtrList {
public:
  List(int size) : PtrList(size) {} 
  inline DATA** Copy() const { return (DATA **)(CopyArray()); }
  inline DATA** MakeArray() { return (DATA **)(CopyAndClearArray()); }
  inline DATA* Item(int n) const { return static_cast<DATA*>(VItem(n)); }
  inline void Append(DATA *x) { VAppend(x); }
  inline void Append(List <DATA> *x) { VAppend(x); }
  inline void InsertAt(int n, DATA* x) { VInsertAt(n, x); }
};

template <class DATA>
inline List <DATA> *Append(List<DATA> *a, List<DATA> *b)
{
  if (NULL==a) return b;
  a->Append(b);
  return a;
}

#endif
