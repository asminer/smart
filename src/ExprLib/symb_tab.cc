
#include "symb_tab.h"
#include "../Utils/textfmt.h"

// #define DEBUG_ADD
// #define DEBUG_REMOVE

#include "../include/splay.h"


// ******************************************************************
// *                                                                *
// *                      symbol_table methods                      *
// *                                                                *
// ******************************************************************

symbol_table::symbol_table(int l2t, int t2l) : table(l2t, t2l)
{
    num_syms = 0;
    num_names = 0;
    FreeList = nullptr;
}

symbol_table::~symbol_table()
{
    for (symbol_list* ptr = PopFree(); ptr; ptr = PopFree()) {
        delete ptr;
    }
}

void symbol_table::addSymbol(symbol* s)
{
    if (!s) return;
    symbol_list* tmp = NewList();
    tmp->Fill(s);
    symbol_list* root = table.Insert(tmp);
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
    std::cerr << "Just added symbol: ";
    s->Print(std::cerr);
    std::cerr << "\n";
    std::cerr << "Symbol table:\n";
    table->Show(std::cerr);
#endif
}

symbol* symbol_table::findSymbol(const char* name)
{
    symbol_list* root = table.Find(name);
    return root ? root->front : nullptr;
}

bool symbol_table::removeSymbol(symbol* s)
{
    symbol_list* list = table.Find(s->Name());
    if (!list)  return false;

    // traverse the list until we find s
    symbol* prev = nullptr;
    symbol* curr;
    for (curr = list->front; curr; curr = curr->Next()) {
        if (curr == s) break;
        prev = curr;
    }
    if (!curr)  return false;  // not found

    // Found; remove it

    if (prev) {
        prev->LinkTo(curr->Next());
    } else {
        list->front = curr->Next();
    }
    num_syms--;
    if (!list->front) {
        list = table.Remove(list);
        delete list;
        num_names--;
    }
    DCASSERT(table->NumElements() == num_names);
#ifdef DEBUG_REMOVE
    std::cerr << "Just removed symbol: ";
    s->Print(std::cerr);
    std::cerr << "\n";
    std::cerr << "Symbol table:\n";
    table->Show(std::cerr);
#endif
    return true;
}

symbol* symbol_table::pop()
{
    if (num_syms != num_names)  return nullptr;  // definitely chaining.
    if (num_syms < 1)           return nullptr;  // empty table
    symbol_list* foo = table.GetItem(num_syms-1);
    if (!foo)  return nullptr;  // hmmm, stack underflow?
    symbol* find = foo->front;
    if (find->Next())   return nullptr;  // definitely chaining!

    // ok, we can remove this safely.
    num_syms--;
    num_names--;
    foo->front = nullptr;
    foo = table.Remove(foo);
    RecycleList(foo);

#ifdef DEBUG_REMOVE
    std::cerr << "Just popped symbol: ";
    find->Print(std::cerr);
    std::cerr << "\n";
    std::cerr << "Symbol table:\n";
    table->Show(std::cerr);
#endif
    return find;
}

symbol* symbol_table::getItem(unsigned i) const
{
    if (num_syms != num_names)  return nullptr;   // definitely chaining.
    if (num_syms < 1)           return nullptr;   // empty table
    symbol_list* foo = table.GetItem(i);
    if (!foo)  return nullptr;  // hmmm, stack underflow?
    symbol* find = foo->front;
    if (find->Next())   return nullptr;  // definitely chaining!
    return find;
}

void symbol_table::copyToArray(const symbol** list)
{
    if (!list)  return;
    symbol_list** allsyms = new symbol_list*[num_names];
    table.CopyToArray(allsyms);
    for (unsigned i=0; i<num_names; i++) {
        list[i] = allsyms[i]->front;
        DCASSERT(list[i]);
    }
    delete[] allsyms;
}

void symbol_table::documentSymbols(doc_formatter &df, const char* keyword)
{
    // TBD - use a splay traversal here
    const symbol** list = new const symbol*[num_names];
    copyToArray(list);
    for (unsigned i=0; i<num_names; i++) {
        const symbol* chain = list[i];
        DCASSERT(chain);
        if (!df.Matches(chain->Name(), keyword))  continue;
        // traverse the chain, show documentation for each
        for (; chain; chain = chain->Next()) {
            df.Out() << "\n";
            chain->PrintDocs(df, keyword);
        }
    }
    delete[] list;
}


// ******************************************************************
// *                                                                *
// *                      symbol_list  methods                      *
// *                                                                *
// ******************************************************************

symbol_table::symbol_list::symbol_list()
{
    name = nullptr;
    front = nullptr;
}

void symbol_table::symbol_list::Fill(symbol* f)
{
    name = f->Name();
    front = f;
}

void symbol_table::symbol_list::Fill(const char* n)
{
    name = n;
    front = nullptr;
}

int symbol_table::symbol_list::Compare(const char* name2) const
{
    if ( (0==name) && (0==name2) )  return 0;
    if (0==name)                    return -1;
    if (0==name2)                   return 1;
    return strcmp(name, name2);
}

void symbol_table::symbol_list::Show(std::ostream &s) const
{
    if (name)   s << name;
    else        s << "no name";
    s << " : ";
    for (symbol* ptr=front; ptr; ptr=ptr->Next()) {
        ptr->Print(s);
        s << ", ";
    }
}

