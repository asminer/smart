
// $Id$

#include "tables.h"
#include "../list.h"
#include <string.h>

// ==================================================================
// |                       PtrTable  methods                        |
// ==================================================================

PtrTable::PtrTable() 
{
    splaywrapper = new SplayWrap <splayitem>;
    root = NULL;
}

PtrTable::~PtrTable()
{
  delete splaywrapper;
  // delete nodes, not sure how...
}

void* PtrTable::FindName(const char* n)
{
    splayitem tmp(n, NULL);
    int foo = splaywrapper->Splay(root, &tmp);
    if (foo!=0) return NULL;
    splayitem *bar = (splayitem*) root->data;
    return bar->ptr;
}

void PtrTable::AddNamePtr(const char* n, void *p)
{
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

// ==================================================================

int Compare(PtrTable::splayitem *a, PtrTable::splayitem *b)
{
  return strcmp(a->name, b->name);
}


void InsertFunction(PtrTable *t, function *f)
{
  if (NULL==f) return;
  void* x = t->FindName(f->Name());
  List <function> *foo = (List <function> *) x;
  if (NULL==x) {
    foo = new List <function> (2);
    t->AddNamePtr(f->Name(), foo);
  }
  foo->Append(f);
}

