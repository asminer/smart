
// $Id$

/*  
    Replacement for linked lists of pointers: use arrays!
*/

#ifndef LIST_H
#define LIST_H

// for memcpy
#include <string.h>

class PtrList {
  void** data;
  int size;
  int last;
protected:
  void Resize(int newsize) {
    void ** foo = (void**) realloc(data, newsize*sizeof(void*));
    if (newsize && (NULL==foo)) {
      Internal.Start(__FILE__, __LINE__);
      Internal << "Memory overflow on List resize\n";
      Internal.Stop();
    }
    data = foo;
    size = newsize;
  }
public:
  PtrList(int inits) {
    data = NULL;
    Resize(inits);
    last = 0;
  }
  PtrList(void** d, int s, int l) {
    data = d;
    size = s;
    last = l;
  }
  ~PtrList() {
    Resize(0);
  }
  inline int Length() const { return last; }
  inline void Pop() { if (last) last--; }
  void** VCopy() const {
    if (0==last) return NULL;
    void** thing = new void* [Length()];
    memcpy(thing, data, last * sizeof(void*));
    return thing;
  };
  /** Equivalent to Copy followed by Clear.
      Since this trashes our copy, this should be used
      when we were going to delete the original anyway.
  */
  void** VMakeArray() {
    if (0==last) return NULL;
    Resize(last);
    void **ret = data;
    size = last = 0;
    data = NULL;
    return ret;
  }

  inline void* VItem(int n) const { 
    DCASSERT(n<last);
    DCASSERT(n>=0);
    DCASSERT(data);
    return data[n]; 
  }

  void VInsertAt(int n, void* x) {
    DCASSERT(n>=0);
    DCASSERT(n<=last);
    DCASSERT(data);
    if (last >= size) Resize(2*size);
    for (int i = last; i>n; i--) {
      data[i] = data[i-1];
    }
    data[n] = x;
    last++;
  }

  void VAppend(void* x) {
    if (last>=size) Resize(2*size);
    data[last] = x;
    last++;
  }

  void VAppend(PtrList *x) {
    if (x) {
      while (last + x->last >= size) size*=2;
      Resize(size);
      // risky....
      memcpy(data+last, x->data, x->last * sizeof(void*));
      last += x->last;
      delete x;
    }
  }

  void Clear() {
    last = 0;
  }
};

template <class DATA> 
class List : public PtrList {
public:
  List(int size) : PtrList(size) {} 
  List(DATA** a, int size, int last) : PtrList((void**)a, size, last) { }
  inline DATA** Copy() const { return (DATA **)(VCopy()); }
  inline DATA** MakeArray() { return (DATA **)(VMakeArray()); }
  inline DATA* Item(int n) const { return static_cast<DATA*>(VItem(n)); }
  inline void Append(DATA *x) { VAppend(x); }
  inline void Append(List <DATA> *x) { if (x) { VAppend(x); delete x; } }
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
