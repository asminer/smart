
#ifndef SYMTABS_H
#define SYMTABS_H

#include "symbols.h"
#include "../Utils/splay.h"

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
        inline symbol* findSymbol(const char* name) {
            const_string CS(name);
            return smart_cast <symbol*> (table.find(&CS));
        }

        /// Traverse the symbol table
        inline void traverse(shared_visitor &t) const {
            table.traverse(t);
        }

        const splayOfShared& getTable() const { return table; }

        /// The "global" symbol table.
        static inline symbol_table& global() {
            if (!_global) _global = new symbol_table;
            DCASSERT(_global);
            return *_global;
        }

        /// Add a symbol to the global symbol table.
        static inline void addGlobal(symbol* s) {
            global().addSymbol(s);
        }

        /// Find symbols in the global symbol table.
        static symbol* findGlobal(const char* name) {
            return global().findSymbol(name);
        }


        /** The common model symbol table.
            Similar to global(), except this is for functions
            that can appear inside a model, either for measure
            computation or other things.
            These will actually be copied into each model's
            own symbol table based on criteria.
        */
        static inline symbol_table& allModelsTable() {
            if (!_allModels) _allModels = new symbol_table;
            DCASSERT(_allModels);
            return *_allModels;
        }

        /// Add a symbol to the common model symbol table.
        static inline void addToAllModels(symbol* s) {
            allModelsTable().addSymbol(s);
        }

    private:
        unsigned num_syms;
        unsigned num_names;
        splayOfShared table;
        static symbol_table* _global;
        static symbol_table* _allModels;
};


#endif
