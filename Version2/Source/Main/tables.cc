
// $Id$

#include "tables.h"
#include "../list.h"
#include <string.h>

//#define TABLE_DEBUG

// ==================================================================
// |                       PtrTable  methods                        |
// ==================================================================

PtrTable::PtrTable() 
{
    splaywrapper = new SplayWrap <splayitem>;
    root = NULL;
    node_count = 0;
    node_pile = new Manager <PtrSplay::node> (16);
    splay_pile = new Manager <splayitem> (16);
}

PtrTable::~PtrTable()
{
  delete splaywrapper;
  delete splay_pile;
  delete node_pile;
}

void* PtrTable::FindName(const char* n)
{
    splayitem tmp(n, NULL);
    int foo = splaywrapper->Splay(root, &tmp);
    if (foo!=0) return NULL;
    splayitem *bar = (splayitem*) root->data;
    return bar->ptr;
}

bool PtrTable::ReplaceNull(const char* n, void *p)
{
  splayitem tmp(n, NULL);
  int foo = splaywrapper->Splay(root, &tmp);
  if (foo!=0) return false; // not found
  splayitem *bar = (splayitem*) root->data;
  if (bar->ptr) return false;  // not null
  bar->ptr = p;
  return true;
}

void PtrTable::AddNamePtr(const char* n, void *p)
{
    DCASSERT(n);
  //  splayitem *key = new splayitem(n, p);
    splayitem *key = splay_pile->NewObject();
    key->Set(n, p);
    int foo = splaywrapper->Splay(root, key);
    DCASSERT(foo!=0);
    // PtrSplay::node *x = new PtrSplay::node;
    PtrSplay::node *x = node_pile->NewObject();
    x->data = key;
    if (foo>0) {
      // root > x
      x->right = root; 
      if (root) {
        x->left = root->left;
        root->left = NULL;
      } else {
	x->left = NULL;
      }
    } else {
      // root < x
      x->left = root; 
      if (root) {
        x->right = root->right;
        root->right = NULL;
      } else {
	x->right = NULL;
      }
    }
    root = x;
    node_count++;
}

// ==================================================================

int Compare(PtrTable::splayitem *a, PtrTable::splayitem *b)
{
  return strcmp(a->name, b->name);
}


void InsertFunction(PtrTable *t, function *f)
{
  if (NULL==f) return;
#ifdef TABLE_DEBUG
  Output << "Adding function " << f << " to table\n";
#endif
  void* x = t->FindName(f->Name());
  List <function> *foo = (List <function> *) x;
  if (NULL==x) {
#ifdef TABLE_DEBUG
    Output << "New name entry\n";
#endif
    foo = new List <function> (2);
    t->AddNamePtr(f->Name(), foo);
  }
  foo->Append(f);
}

List <function> *FindFunctions(PtrTable *t, const char* n)
{
  if (NULL==n) return NULL;
  void* x = t->FindName(n);
  return (List <function> *)x;
}


