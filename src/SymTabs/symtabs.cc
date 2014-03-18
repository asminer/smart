
// $Id$

#include <string.h>
#include <stdlib.h>
#include "../ExprLib/symbols.h"
#include "symtabs.h"
#include "../Streams/streams.h"

// #define DEBUG_ADD
// #define DEBUG_REMOVE

#ifdef DEBUG_ADD
#define DEBUG
#endif
#ifdef DEBUG_REMOVE
#define DEBUG
#endif

#ifdef DEBUG
DisplayStream Debug(stderr);
#endif

#include "../include/splay.h"

// ******************************************************************
// *                      symbol_table methods                      *
// ******************************************************************

symbol_table::symbol_table()
{
  num_syms = 0;
  num_names = 0;
}

symbol_table::~symbol_table()
{
}

void symbol_table::DocumentSymbols(doc_formatter* df, const char* keyword) const
{
  if (0==df)  return;
  const symbol** list = new const symbol*[num_names];
  CopyToArray(list);
  for (long i=0; i<num_names; i++) {
    const symbol* chain = list[i];
    DCASSERT(chain);
    if (!df->Matches(chain->Name(), keyword))  continue;
    // traverse the chain, show documentation for each
    for (; chain; chain = chain->Next()) {
      df->Out() << "\n";
      chain->PrintDocs(df, keyword);
    }
  }
  delete[] list;
}


// ******************************************************************
// *                       symbol_list struct                       *
// ******************************************************************

/** For a list of symbols with the same name.
    We could simply use type symbol, but this
    allows us to easily change the front of the list.
*/
struct symbol_list {
  /// Name of symbols in this list.
  const char* name;
  /// Front of the list.
  symbol* front;
public:
  symbol_list();
  void Fill(symbol* f);
  void Fill(const char* n);
  int Compare(const char* x) const;
  inline int Compare(const symbol_list* x) const {
    return Compare(x ? x->name : 0);
  }
  void Show(OutputStream& s) const;
};

symbol_list::symbol_list()
{
  name = 0;
  front = 0;
}

void symbol_list::Fill(symbol* f)
{
  name = f->Name();
  front = f;
}

void symbol_list::Fill(const char* n)
{
  name = n;
  front = 0;
}

int symbol_list::Compare(const char* name2) const
{
  if ( (0==name) && (0==name2) )  return 0;
  if (0==name)                    return -1;
  if (0==name2)                   return 1;
  return strcmp(name, name2);
}

void symbol_list::Show(OutputStream& s) const
{
  if (name)   s << name;
  else        s << "no name";
  s << " : ";
  symbol* ptr;
  for (ptr=front; ptr; ptr=ptr->Next()) {
    ptr->Print(s, 0);
    s << ", ";
  }
}

// ******************************************************************
// *                       my_symtable  class                       *
// ******************************************************************

class my_symtable : public symbol_table {
  SplayOfPointers <symbol_list> *table;
  symbol_list* FreeList;
protected:
  inline symbol_list* PopFree() {
    if (0==FreeList)  return 0;
    symbol_list* tmp = FreeList;
    FreeList = (symbol_list*) FreeList->front;
    return tmp;
  }
  inline symbol_list* NewList() {
    symbol_list* tmp = PopFree();
    if (0==tmp)  tmp = new symbol_list;
    return tmp;
  }
  inline void RecycleList(symbol_list* f) {
    f->front = (symbol*) FreeList;
    FreeList = f;
  }
public:
  my_symtable(int l2t, int t2l);
  virtual ~my_symtable();

  virtual void AddSymbol(symbol* s);
  virtual symbol* FindSymbol(const char* name);
  virtual bool RemoveSymbol(symbol* s);
  virtual symbol* Pop();
  virtual symbol* GetItem(int i) const;
  // virtual void Clear();
  virtual void CopyToArray(const symbol** list) const;
};

// ******************************************************************
// *                      my_symtable  methods                      *
// ******************************************************************

my_symtable::my_symtable(int l2t, int t2l) : symbol_table()
{
  table = new SplayOfPointers <symbol_list> (l2t, t2l);
  FreeList = 0;
}

my_symtable::~my_symtable()
{
  delete table;
  for (symbol_list* ptr = PopFree(); ptr; ptr = PopFree()) {
    delete ptr;
  }
}

void my_symtable::AddSymbol(symbol* s)
{
  if (0==s) return;
  symbol_list* tmp = NewList();
  tmp->Fill(s); 
  symbol_list* root = table->Insert(tmp);
  if (root == tmp) {
    // new node in tree
    num_names++;
  } else {
    // existing node, add to list
    s->LinkTo(root->front);
    root->front = s;
    RecycleList(tmp);
  }
  num_syms++;
  DCASSERT(table->NumElements() == num_names);

#ifdef DEBUG_ADD
  Debug << "Just added symbol: ";
  s->Print(Debug, 0);
  Debug << "\n";
  Debug << "Symbol table:\n";
  Debug.flush();
  table->Show(Debug);
  Debug.flush();
#endif
}

symbol* my_symtable::FindSymbol(const char* name)
{
  symbol_list* root = table->Find(name);
  return root ? root->front : 0;
}

bool my_symtable::RemoveSymbol(symbol* s)
{
  symbol_list* list = table->Find(s->Name());
  if (0==list)  return false;

  // traverse the list until we find s
  symbol* prev = 0;
  symbol* curr;
  for (curr = list->front; curr; curr = curr->Next()) {
    if (curr == s) break;
    prev = curr;
  }
  if (0==curr)  return false;  // not found

  if (prev) {
    prev->LinkTo(curr->Next());
  } else {
    list->front = curr->Next();
  }
  num_syms--;
  if (0==list->front) {
    list = table->Remove(list);
    delete list;
    num_names--;
  }
  DCASSERT(table->NumElements() == num_names);
#ifdef DEBUG_REMOVE
  Debug << "Just removed symbol: ";
  s->Print(Debug, 0);
  Debug << "\n";
  Debug << "Symbol table:\n";
  Debug.flush();
  table->Show(Debug);
  Debug.flush();
#endif
  return true;
}

symbol* my_symtable::Pop()
{
  if (num_syms != num_names)  return 0;  // definitely chaining.
  if (num_syms < 1)           return 0;  // empty table
  symbol_list* foo = table->GetItem(num_syms-1);
  if (0==foo)  return 0;  // hmmm, stack underflow?
  symbol* find = foo->front;
  if (find->Next())   return 0;  // definitely chaining!
  
  // ok, we can remove this safely.
  num_syms--;
  num_names--;
  foo->front = 0;
  foo = table->Remove(foo);
  RecycleList(foo);

#ifdef DEBUG_REMOVE
  Debug << "Just popped symbol: ";
  find->Print(Debug, 0);
  Debug << "\n";
  Debug << "Symbol table:\n";
  Debug.flush();
  table->Show(Debug);
  Debug.flush();
#endif
  return find;
}

symbol* my_symtable::GetItem(int i) const
{
  if (num_syms != num_names)  return 0;   // definitely chaining.
  if (num_syms < 1)           return 0;   // empty table
  symbol_list* foo = table->GetItem(i);
  if (0==foo)  return 0;  // hmmm, stack underflow?
  symbol* find = foo->front;
  if (find->Next())   return 0;  // definitely chaining!
  return find;
}

void my_symtable::CopyToArray(const symbol** list) const
{
  if (0==table) return;
  if (0==list)  return;
  symbol_list** allsyms = new symbol_list*[num_names];
  table->CopyToArray(allsyms);
  for (int i=0; i<num_names; i++) {
    list[i] = allsyms[i]->front;
    DCASSERT(list[i]);
  }
  delete[] allsyms;
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

symbol_table* MakeSymbolTable()
{
  return new my_symtable(64, 32);
}
