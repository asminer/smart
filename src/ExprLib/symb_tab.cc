
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
    num_names = 0;
}

symbol_table::~symbol_table()
{
}

void symbol_table::addSymbol(symbol* s)
{
    if (!s) return;
    ++num_syms;
    symbol* root = smart_cast <symbol*> (table.insert(s));
    if (s != root) {
        // Existing node; push s
        s->LinkTo(root);
        table.updateRoot(root, s);
    } else {
        // New node
        ++num_names;
    }

#ifdef DEBUG_ADD
    std::cerr << "Just added symbol: " << *s << "\n";
    std::cerr << "Symbol table:\n";
    table.show(std::cerr);
#endif
}


