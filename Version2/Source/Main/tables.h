
// $Id$

#ifndef TABLES_H
#define TABLES_H

#include "../list.h"
#include "../splay.h"
#include <string.h>

/**
    A symbol table class.
    Basically, a splay tree, where the data is a pair (name, ptr).
    For overloading, ptr should be a list of items.
*/

class PtrTable {
public:
  struct splayitem {
    const char* name;
    void *ptr;
    splayitem(const char *n, void *x) { name = n; ptr = x; }
  };
protected:
  SplayWrap <splayitem> *splaywrapper; 
  PtrSplay::node *root;
public:
  PtrTable() {
    splaywrapper = new SplayWrap <splayitem>;
    root = NULL;
  }
  ~PtrTable();
  void* FindName(const char* n) {
    splayitem tmp(n, NULL);
    int foo = splaywrapper->Splay(root, &tmp);
    if (foo!=0) return NULL;
    splayitem *bar = (splayitem*) root->data;
    return bar->ptr;
  }
  void AddNamePtr(const char* n, void *p) {
    splayitem *key = new splayitem(n, p);
    int foo = splaywrapper->Splay(root, key);
    DCASSERT(foo!=0);
    PtrSplay::node *x = new PtrSplay::node;
    x->data = key;
    if (foo>0) {
      // root > x
      x->right = root; x->left = NULL;
    } else {
      // root < x
      x->left = root; x->right = NULL;
    }
    root = x;
  }
};

int Compare(PtrTable::splayitem *a, PtrTable::splayitem *b)
{
  return strcmp(a->name, b->name);
}

#endif

