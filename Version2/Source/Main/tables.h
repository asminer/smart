
// $Id$

#ifndef TABLES_H
#define TABLES_H

#include "../Language/api.h"
#include "../list.h"
#include "../splay.h"

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
    splayitem(const char *n, void *x) { name = n; ptr = x; }
  };
protected:
  void Traverse(PtrSplay::node *root, tablevisit visit) {
	if (NULL==root) return;
	Traverse(root->left, visit);
	visit(root->data);
  	Traverse(root->right, visit);
  }
protected:
  SplayWrap <splayitem> *splaywrapper; 
  PtrSplay::node *root;
public:
  PtrTable();
  ~PtrTable();
  void* FindName(const char* n);
  void AddNamePtr(const char* n, void *p);
  void Traverse(tablevisit visit) { Traverse(root, visit); }
};

int Compare(PtrTable::splayitem *a, PtrTable::splayitem *b);

/**
    Add function f to the list of functions with that name, to table t.
*/
void InsertFunction(PtrTable *t, function *f);

/**
    Find list of functions with name n, from table t, or NULL if none.
*/
List <function> *FindFunctions(PtrTable *t, const char* n);

#endif

