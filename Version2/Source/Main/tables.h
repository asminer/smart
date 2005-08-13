
// $Id$

#ifndef TABLES_H
#define TABLES_H

#include "../Language/api.h"
#include "../Templates/list.h"
#include "../Templates/splay.h"
#include "../Templates/memmgr.h"

/**
    A symbol table class.
    Basically, a splay tree, where the data is a pair (name, ptr).
    For overloading, ptr should be a list of items.
*/


/** To traverse a symbol table, define a function:

    void MyFunc(void *x)

    which will be called for each symbol table node (splayitem).
*/
typedef void (*tablevisit) (void *ptr);

class PtrTable {
public:
  struct splayitem {
    const char* name;
    void *ptr;
    int left;
    int right;
    splayitem() { }
    splayitem(const char *n, void *x) { name = n; ptr = x; }
    void Set(const char *n, void *x) { name = n; ptr = x; left = right = -1; }
  };
protected:
  splayitem* heap;
  int heapsize;
  int last_item;
  tablevisit visit;
  SplayTree <PtrTable>* splaytable;
  int root;
public:
  PtrTable();
  ~PtrTable();
  bool ContainsName(const char* n);
  void* FindName(const char* n);
  bool ReplaceNull(const char* n, void *p);
  void AddNamePtr(const char* n, void *p);
  void Traverse(tablevisit visit);
  inline int NameCount() const { return last_item; }
  // stuff for splay tree
  inline int Null() { return -1; }
  inline int getLeft(int h) { return heap[h].left; }
  inline void setLeft(int h, int n) { heap[h].left = n; }
  inline int getRight(int h) { return heap[h].right; }
  inline void setRight(int h, int n) { heap[h].right = n; }
  inline int Compare(int h1, int h2) {
    return strcmp(heap[h1].name, heap[h2].name);
  }
  inline void Visit(int h) {
    DCASSERT(visit);
    visit(heap+h);
  }
};

/**
    Add function f to the list of functions with that name, to table t.
*/
void InsertFunction(PtrTable *t, function *f);

/**
    Find list of functions with name n, from table t, or NULL if none.
*/
List <function> *FindFunctions(PtrTable *t, const char* n);


#endif

