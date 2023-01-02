
#include "symb_tab.h"
#include "../Utils/textfmt.h"

// #define DEBUG_ADD
// #define DEBUG_REMOVE


symbol_table* symbol_table::_global = nullptr;
symbol_table* symbol_table::_allModels = nullptr;

// ******************************************************************
// *                                                                *
// *                      symbol_table methods                      *
// *                                                                *
// ******************************************************************

symbol_table::symbol_table(int l2t, int t2l) : table(l2t, t2l)
{
    num_syms = 0;
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
    symbol_list* root = smart_cast <symbol_list*> (table.insert(tmp));
    if (root != tmp) {
        // existing node, add to list
        s->LinkTo(root->front);
        root->front = s;
        RecycleList(tmp);
    }
    num_syms++;

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
    const_string CS(name);
    symbol_list* root = smart_cast <symbol_list*> (table.find(&CS));
    return root ? root->front : nullptr;
}

bool symbol_table::removeSymbol(symbol* s)
{
    const_string CS(s->Name());
    symbol_list* list = smart_cast <symbol_list*> (table.find(&CS));
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
        list = smart_cast <symbol_list*> (table.remove(list));
        delete list;
        num_names--;
    }
#ifdef DEBUG_REMOVE
    std::cerr << "Just removed symbol: ";
    s->Print(std::cerr);
    std::cerr << "\n";
    std::cerr << "Symbol table:\n";
    table->Show(std::cerr);
#endif
    return true;
}

/*
symbol* symbol_table::pop()
{
    if (num_syms != table.numElements()) return nullptr;  // definitely chaining.
    if (num_syms < 1)           return nullptr;  // empty table
    symbol_list* foo = smart_cast <symbol_list*> (table.getElement(num_syms-1));
    if (!foo)  return nullptr;  // hmmm, stack underflow?
    symbol* find = foo->front;
    if (find->Next())   return nullptr;  // definitely chaining!

    // ok, we can remove this safely.
    num_syms--;
    foo->front = nullptr;
    foo = smart_cast <symbol_list*> (table.remove(foo));
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
    symbol_list* foo = smart_cast <symbol_list*> (table.getElement(i));
    if (!foo)  return nullptr;  // hmmm, stack underflow?
    symbol* find = foo->front;
    if (find->Next())   return nullptr;  // definitely chaining!
    return find;
}
*/

/*
void symbol_table::copyToArray(const symbol** list)
{
    if (!list)  return;
    const unsigned num_names = table.numElements();
    symbol_list** allsyms = new symbol_list*[num_names];
    copy_traversal <symbol_list> T(allsyms, num_names);
    table.traverse(T);
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
*/

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

bool symbol_table::symbol_list::Print(std::ostream &s, int width) const
{
    if (name)   s << name;
    else        s << "no name";
    s << " : ";
    for (symbol* ptr=front; ptr; ptr=ptr->Next()) {
        ptr->Print(s);
        s << ", ";
    }
    return true;
}

int symbol_table::symbol_list::Compare(const shared_object* s) const
{
    const const_string* cs = dynamic_cast <const const_string*> (s);
    if (cs) {
        if (name && cs->getStr()) return strcmp(name, cs->getStr());
        if (name) return 1;
        if (cs->getStr()) return -1;
        return 0;
    }
    const symbol_list* sl = dynamic_cast <const symbol_list*> (s);
    if (sl) {
        if (name && sl->name) return strcmp(name, sl->name);
        if (name) return 1;
        if (sl->name) return -1;
        return 0;
    }
    const shared_object* t = this;
    return t - s;
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

