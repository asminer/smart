
// $Id$

#include "tables.h"
#include <string.h>

//#define TABLE_DEBUG

int Compare(PtrTable::splayitem *a, PtrTable::splayitem *b);


// ==================================================================
// |                       PtrTable  methods                        |
// ==================================================================

PtrTable::PtrTable() 
{
  heapsize = 256;
  heap = (splayitem*) malloc(heapsize * sizeof(splayitem));
  last_item = 0;
  root = -1;
  splaytable = new SplayTree <PtrTable> (this);
}

PtrTable::~PtrTable()
{
  delete splaytable;
  free(heap);
}

bool PtrTable::ContainsName(const char* n) 
{
  heap[last_item].name = n;
  return (0==splaytable->Splay(root, last_item));
}

void* PtrTable::FindName(const char* n)
{
  heap[last_item].name = n;
  int cmp = splaytable->Splay(root, last_item);
  if (cmp!=0) return NULL;
  return heap[root].ptr;
}

bool PtrTable::ReplaceNull(const char* n, void *p)
{
  heap[last_item].name = n;
  int cmp = splaytable->Splay(root, last_item);
  if (cmp!=0) return false;  // not found
  if (heap[root].ptr) return false; // not null
  heap[root].ptr = p;
  return true; 
}

void PtrTable::AddNamePtr(const char* n, void *p)
{
  DCASSERT(n);
  heap[last_item].Set(n, p);
  root = splaytable->Insert(root, last_item);
  DCASSERT(root==last_item);
  // expand
  last_item++;
  if (last_item>=heapsize) {
    heapsize += 256;
    heap = (splayitem*) realloc(heap, heapsize * sizeof(splayitem));
    if (NULL==heap)
      OutOfMemoryError("Symbol table overflow");
  }
}

void PtrTable::Traverse(tablevisit v)
{
  visit = v;
  if (visit) splaytable->InorderTraverse(root);
}

// ==================================================================


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
  if (NULL==t) return NULL;
  void* x = t->FindName(n);
  return (List <function> *)x;
}


