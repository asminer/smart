
/**
  \file formalism.h
  The formalism interface is specified here.
  This file should be included only if you are
  building a modeling formalism.
  (Or, if you are the formalism manager.)
*/

#ifndef FORMALISM_H
#define FORMALISM_H

#include "../include/list.h"
#include "../Utils/ordarray.h"
#include "symb_tab.h"
#include "type.h"

class model_def;
class location;

class formalism : public simple_type {
public:
    formalism(const char* n, const char* sd, const char* ld);
    virtual ~formalism();

    virtual void printDocs(doc_formatter &df) const; // overrides simple_type

    // Formalism-specific symbols
    inline void addSymbol(symbol* s) {
        DCASSERT(symb_tree);
        if (s) symb_tree->addSymbol(s);
    }
    /// Add common functions, and finalize the symbol table.
    void finish();

    /// Find a symbol, after finalization.
    inline symbol* findSymbol(const char* n) const {
        const_string C(n);
        return symb_list ? symb_list->find(&C) : nullptr;
    }

    //
    // Traverse both functions and identifiers
    //
    inline void traverseSymbols(shared_visitor &t) const {
        if (symb_list) {
            symb_list->traverse(t);
            return;
        }
        if (symb_tree) {
            symb_tree->traverse(t);
            return;
        }
    }

    //
    // Required in derived classes:
    //

    virtual model_def* makeNewModel(const location& W, char* name,
            symbol** formals, int np) const = 0;

    virtual bool canDeclareType(const type* vartype) const = 0;
    virtual bool canAssignType(const type* vartype) const = 0;

    // Default behavior is provided for this one:
    virtual bool isLegalMeasureType(const type* mtype) const;


protected:
    /// Call this in the constructor if the formalism
    /// should include common CTL measures.
    inline void includeCTL() {
        include_ctl = true;
    }

    /// Call this in the constructor if the formalism
    /// should include stochastic measures.
    inline void includeStochastic() {
        include_stoch = true;
    }

    /// Call this in the constructor if the formalism
    /// should include discrete constraint problem measures.
    inline void includeDCP() {
        include_dcp = true;
    }

private:
    /// Symbol table, during construction
    symbol_table* symb_tree;
    /// Fixed symbol table
    orderedArray <symbol>* symb_list;

    bool include_ctl;       // default: false
    bool include_stoch;     // default: false
    bool include_dcp;       // default: false

    friend class copy_into_formalism;
};

#endif
