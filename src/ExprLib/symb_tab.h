
#ifndef SYMTABS_H
#define SYMTABS_H

#include "symbols.h"
#include "../include/splay.h"

/**
    Symbol table interface.
*/
class symbol_table {
    public:
        symbol_table(int l2t=64, int t2l=32);   // splay parameters
        ~symbol_table();

        /// The number of symbols stored in the table.
        inline unsigned numSymbols() const { return num_syms; }

        /// The number of unique names stored in the table.
        inline unsigned numNames() const { return num_names; }

        /// Add the symbol s to the table.
        void addSymbol(symbol* s);

        /// Find a (list of) symbol matching the given name, otherwise null.
        symbol* findSymbol(const char* name);

        /// Remove the given symbol.  Return true if the item was in the table.
        bool removeSymbol(symbol* s);

        /** Pop and return the last added symbol.
            This only works if there is no chaining, i.e.,
            symbol names are unique;
            if this might not be the case, we return a null pointer.
        */
        symbol* pop();

        /** Return the ith symbol added.
            Like pop(), this works only if there is no chaining.
                @param  i  The symbol number to return.
                @return 0, if i is out of range; a valid symbol, otherwise.
        */
        symbol* getItem(unsigned i) const;

        /** Grab a copy of all symbols, in order.
            Used primarily for documentation.
        */
        void copyToArray(const symbol** list);

        void documentSymbols(doc_formatter &df, const char* keyword);

    private:
        /// The head of a list of symbols, with this name.
        struct symbol_list {
                /// Name of all symbols in this list.
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
                void Show(std::ostream &s) const;
        };

    private:
        unsigned num_syms;
        unsigned num_names;
        SplayOfPointers <symbol_list> table;
        symbol_list* FreeList;

    private:    // helpers
        inline symbol_list* PopFree() {
            if (!FreeList) return nullptr;
            symbol_list* tmp = FreeList;
            FreeList = (symbol_list*) FreeList->front;
            return tmp;
        }
        inline symbol_list* NewList() {
            symbol_list* tmp = PopFree();
            if (!tmp) tmp = new symbol_list;
            return tmp;
        }
        inline void RecycleList(symbol_list* f) {
            f->front = (symbol*) FreeList;
            FreeList = f;
        }
};

//
// TO DO:
//
//   (1) Make sure this uses the new splay class
//   (2) Static member for the global symbol table
//   (3) Static member for the global measure table (common measures)
//          search this after searching the per-model measure table.
//

// symbol_table* MakeSymbolTable();  // any params necessary?

#endif
