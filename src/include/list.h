
#ifndef LIST_H
#define LIST_H

#include "defines.h"
#include <stdlib.h>  // for realloc
#include <string.h>  // for memcpy

class voidlist {
  void** data;
  int alloc;
  int last;
protected:
  inline bool Resize(int ns) {
    if (ns == alloc)  return 1;
    void** newdata = (void**) realloc(data, ns * sizeof(void*));
    if (ns) {
      if (0==newdata)  return 0;
      data = newdata;
      alloc = ns;
    } 
    return 1;
  }
public:
  voidlist() {
    data = 0;
    alloc = 0;
    last = 0;
  }
  ~voidlist() {
    free(data);
  }
  inline int Length() const { return last; }
  inline bool Append(void* x) {
    if (last >= alloc) {
      int ns;
      if (alloc < 1)          ns = 16;
      else if (alloc < 1024)  ns = alloc*2;
      else                    ns = alloc + 1024;
      if (!Resize(ns))        return false;
    }
    data[last] = x;
    last++;
    return true;
  }
  inline void* Item(int n) {
    CHECK_RANGE(0, n, last);
    return data[n];
  }
  inline const void* ReadItem(int n) const {
    CHECK_RANGE(0, n, last);
    return data[n];
  }
  inline void Clear() {
    last = 0;
  }
  inline void Compact() {
    Resize(last);
  }
  inline void** CopyIntoArray() const {
    if (0==last)  return 0;
    void** foo = (void**) malloc(last * sizeof(void*));
    memcpy(foo, data, last * sizeof(void*));
    return foo;
  }
  inline void** CopyAndClear() {
    Compact();
    void** foo = data;
    last = 0;
    data = 0;
    alloc = 0;
    return foo;
  }
};

template <class DATA>
class List {
  voidlist L;
public:
  inline int Length() const { 
    return L.Length(); 
  }
  inline bool Append(DATA* x) { 
    return L.Append(x); 
  }
  inline DATA* Item(int n) { 
    return (DATA*) L.Item(n); 
  }
  inline const DATA* ReadItem(int n) const {
    return (const DATA*) L.ReadItem(n);
  }
  inline void Clear() {
    L.Clear();
  }
  inline void Compact() {
    L.Compact();
  }
  inline DATA** CopyIntoArray() const {
    return (DATA**) L.CopyIntoArray();
  }
  inline DATA** CopyAndClear() {
    return (DATA**) L.CopyAndClear();
  }
};


template <class DATA>
class ObjectList {
  DATA* data;
  int alloc;
  int last;
protected:
  inline bool Resize(int ns) {
    if (ns == alloc)  return 1;
    DATA* newdata = (DATA*) realloc(data, ns * sizeof(DATA));
    if (ns) {
      if (0==newdata)  return 0;
      data = newdata;
      alloc = ns;
    } 
    return 1;
  }
public:
  ObjectList() {
    data = 0;
    alloc = 0;
    last = 0;
  }
  ~ObjectList() {
    free(data);
  }
  inline int Length() const { return last; }
  inline bool Append(DATA x) {
    if (last >= alloc) {
      int ns;
      if (alloc < 1)    ns = 16;
      else if (alloc < 1024)  ns = alloc*2;
      else      ns = alloc + 1024;
      if (!Resize(ns))    return false;
    }
    data[last] = x;
    last++;
    return true;
  }
  inline DATA Item(int n) const {
    CHECK_RANGE(0, n, last);
    return data[n];
  }
  inline void Clear() {
    last = 0;
  }
  inline void Compact() {
    Resize(last);
  }
  inline DATA* CopyIntoArray() const {
    if (0==last)  return 0;
    DATA* foo = (DATA*) malloc(last * sizeof(DATA));
    memcpy(foo, data, last * sizeof(DATA));
    return foo;
  }
  inline DATA* CopyAndClear() {
    Compact();
    DATA* foo = data;
    last = 0;
    data = 0;
    alloc = 0;
    return foo;
  }
};


#endif
