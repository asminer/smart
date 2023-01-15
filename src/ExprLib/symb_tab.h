
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

        /// Remove the given symbol.  Return true if the item was in the table.
        // bool removeSymbol(symbol* s);

        /// Traverse the symbol table
        inline void traverse(shared_visitor &t) const {
            table.traverse(t);
        }

        /** Pop and return the last added symbol.
            This only works if there is no chaining, i.e.,
            symbol names are unique;
            if this might not be the case, we return a null pointer.
        */
//        symbol* pop();

        /** Return the ith symbol added.
            Like pop(), this works only if there is no chaining.
                @param  i  The symbol number to return.
                @return 0, if i is out of range; a valid symbol, otherwise.
        */
        // symbol* getItem(unsigned i) const;

        /** Grab a copy of all symbols, in order.
            Used primarily for documentation.
        */
//        void copyToArray(const symbol** list);

 //       void documentSymbols(doc_formatter &df, const char* keyword);

        const splayOfShared& getTable() const { return table; }

        /// The "global" symbol table.
        static inline symbol_table& global() {
            if (!_global) _global = new symbol_table;
            DCASSERT(_global);
            return *_global;
        }

        /// Add a symbol to the global symbol table.
        static inline void addGlobal(symbol* s) {
            std::cout << "  adding global " << *s << "\n";
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

        /*
    private:
        /// The head of a list of symbols, with this name.
        struct symbol_list : public shared_object {
                /// Name of all symbols in this list.
                const char* name;
                /// Front of the list.
                symbol* front;
            public:
                symbol_list();

                // Required for shared_object
                virtual bool Print(std::ostream &s, int width=0) const;
                virtual int Compare(const shared_object* s) const;

                void Fill(symbol* f);
                void Fill(const char* n);
        };

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
        */

    private:
        unsigned num_syms;
        unsigned num_names;
        splayOfShared table;
        // symbol_list* FreeList;
        static symbol_table* _global;
        static symbol_table* _allModels;
};


#endif
