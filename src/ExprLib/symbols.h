
#ifndef SYMBOLS_H
#define SYMBOLS_H

/** \file symbols.h

    Base class for symbols.  These are simply expressions with names.

*/

#include "expr.h"

class doc_formatter;

/**   The base class of all symbols.
      That includes formal parameters, for loop iterators,
      functions, and model objects like states, places, and transitions.

      Note: we derive symbols from expressions because it
      greatly simplifies for-loop iterators
      (otherwise you have an expression wrapped around
      an iterator, and that is unnecessary overhead.)
*/

class symbol : public expr {
    private:
        /// The symbol name.
        shared_string* name;
        /// Should we substitute our value?  Default: true.
        bool substitute_value;
        /// Next item in the symbol table.
        symbol* next;
        /// List of symbols waiting on us
        List <symbol> *waitlist;
    public:
        symbol(const location &W, const type* t, char* n);
        symbol(const location &W, typelist* t, char* n);
        symbol(const symbol* wrapper);
    protected:
        virtual ~symbol();
    public:
        inline symbol* Next() const { return next; }
        inline void LinkTo(symbol* n) { next = n; }

        virtual const char* Name() const;
        virtual shared_object* SharedName() const;
        virtual void Rename(shared_object* newname);

        /** Change our substitution rule.
                @param sv   If sv is true, whenever an expression calls
                            "Substitute", we will replace ourself with
                            a constant equal to our current value.
                            If sv is false, whenever an expression calls
                            "Substitute", we will copy ourself.
        */
        inline void SetSubstitution(bool sv) { substitute_value = sv; }

        inline bool WillSubstitute() const { return substitute_value; }

        virtual bool Print(std::ostream &s, int width=0) const;
        virtual int Compare(const shared_object* o) const;

        virtual void Traverse(traverse_data &x);

        /// Display documentation for this symbol.
        virtual void PrintDocs(doc_formatter &df, const char* keyword) const;

        /// Add a symbol to our waiting list.
        void addToWaitList(symbol* w);

        /** This is how symbols on a waiting list are notified.
            See method notifyList() below.
            Default behavior is to print debugging info.
                @param  parent  The symbol that is notifying us
                                (we were on their waiting list).
        */
        virtual void notifyFrom(const symbol* parent);

        /// Check if a symbol could be waiting for us (perhaps indirectly).
        bool couldNotify(const symbol* s) const;

        /// Notify everyone on our waiting list, and destroy the list.
        void notifyList();

    public:
        /** Make a "constant" (function with no parameters) symbol,
            not within a converge block.
                @param  W     Where defined.
                @param  t     Type of the variable.
                @param  name  Name of the variable.
                @param  rhs   Expression to assign to this symbol.
                @param  deps  List of symbols that must be
                              "computed" before this one.
                @return 0, if some error occurred.
                        A new expression, otherwise.
        */
        static symbol* makeConstant(const location& W, const type* t,
            char* name, expr* rhs, List <symbol> *deps=nullptr);


        /** Make a "constant" (function with no parameters) symbol,
            not within a converge block.
                @param  w     Wrapper symbol around this one.
                @param  rhs   Expression to assign to this symbol.
                @param  deps  List of symbols that must be
                              "computed" before this one.
                @return 0, if some error occurred.
                        A new expression, otherwise.
        */
        static symbol* makeConstant(const symbol* w, expr* rhs,
                List <symbol> *deps=nullptr);

        /** Make an iterator variable.
            I.e., a variable used as an array index, also as
            for-loop iterators.
            Implemented in forloops.cc.
                @param  W       Where defined.
                @param  t       Type of the variable.
                @param  name    Name of the variable.
                @param  vals    Set of values for the variable.
                @return 0,      if some error occurred.
                        A new expression, otherwise.
        */
        static symbol* makeIterator(const location& W, const type* t,
            char* name, expr* vals);

        /** Make a new array.
            Implemented in arrays.cc.

                @param  W       Where defined.
                @param  t       Type of the array.
                @param  name    Name of the array.
                @param  indexes List of iterators, which define the "shape"
                                of the array.  Each iterator must have been
                                created by calling MakeIterator().
                @param  dim     Length of the list of indexes.
                                Can be considered the dimension of the array.
                @return 0,      if some error occurred (will make noise).
                                A new array, otherwise.
        */
        static symbol* makeArray(const location& W, const type* t,
            char* n, symbol** indexes, int dim);

        /** Make a variable within a converge block.
            Implemented in converge.cc.

                @param  W   Where defined.
                @param  t   Type of the variable.
                @param  n   Name of the variable.
                @return 0,  if some error occurred (will make noise).
                        A new variable, otherwise.
        */
        static symbol* makeCvgVar(const location& W, const type* t, char* n);


        /** Make a symbol within a model definition.
            The symbol is simply a "placeholder" that will be replaced
            each time the model is instantiated.
            Implemented in mod_vars.cc.

                @param  W     Where defined.
                @param  t     Type of the symbol.
                @param  name  Name of the symbol.

                @return  a new placeholder symbol.
        */
        static symbol* makeModelSymbol(const location& W, const type* t,
                char* name);


        /** Make an array within a model definition.
            The array is simply a "placeholder" that will be replaced
            each time the model is instantiated.
            Implemented in mod_vars.cc.

                @param  W     Where defined.
                @param  t       Type of the array.
                @param  name    Name of the array.
                @param  indexes List of iterators, which define the "shape"
                                of the array.  Each iterator must have been
                                created by calling MakeIterator().
                @param  dim     Length of the list of indexes.
                                Can be considered the dimension of the array.
                @return  0,     if some error occurred (will make noise).
                                A new array, otherwise.
        */
        static symbol* makeModelArray(const location& W, const type* t,
                char* name, symbol** indexes, int dim);


    public:
        //
        // Useful for symbol tables, that store chains of symbols.
        // This visitor will visit all symbols in a chain, and
        // apply the inner visitor V.
        //
        class chain_visit : public shared_visitor {
                shared_visitor &V;
            public:
                chain_visit(shared_visitor &v) : V(v) { };
                virtual void visit(shared_object* obj) {
                    symbol* s = dynamic_cast <symbol*> (obj);
                    if (!s) return;
                    for (; s; s=s->Next()) {
                        V.visit(s);
                    }
                }
        };
};


#endif
