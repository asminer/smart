
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
  ~PtrList() {
    Resize(0);
  }
  inline int Length() const { return last; }
  inline void Pop() { if (last) last--; }
  void** Copy() const {
    if (0==last) return NULL;
    void** thing = new void* [Length()];
    memcpy(thing, data, last * sizeof(void*));
    return thing;
  };
  /** Equivalent to Copy followed by Clear.
      Since this trashes our copy, this should be used
      when we were going to delete the original anyway.
  */
  void** MakeArray() {
    if (0==last) return NULL;
    Resize(last);
    void **ret = data;
    size = last = 0;
    data = NULL;
    return ret;
  }

  inline void* Item(int n) const { 
    DCASSERT(n<last);
    DCASSERT(n>=0);
    DCASSERT(data);
    return data[n]; 
  }

  void InsertAt(int n, void* x) {
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

  void Append(void* x) {
    if (last>=size) Resize(2*size);
    data[last] = x;
    last++;
  }

  void Append(PtrList *x) {
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
class List {
  PtrList *p;
public:
  List(int size) { p = new PtrList(size); }  
  ~List() { delete p; }
  inline int Length() const { return p->Length(); }
  inline void Pop() { p->Pop(); }
  inline DATA** Copy() const { return (DATA **)(p->Copy()); }
  inline DATA** MakeArray() { return (DATA **)(p->MakeArray()); }
  inline DATA* Item(int n) const { return static_cast<DATA*>(p->Item(n)); }
  inline void Append(DATA *x) { p->Append(x); }
  inline void Append(List <DATA> *x) { if (x) { p->Append(x->p); delete x; } }
  inline void InsertAt(int n, DATA* x) { p->InsertAt(n, x); }
  inline void Clear() { p->Clear(); }
};

template <class DATA>
inline List <DATA> *Append(List<DATA> *a, List<DATA> *b)
{
  if (NULL==a) return b;
  a->Append(b);
  return a;
}

#endif
