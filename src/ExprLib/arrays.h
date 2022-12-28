
#ifndef ARRAYS_H
#define ARRAYS_H

/** \file arrays.h

    User-defined arrays.

    Trying for a simple, minimalist interface for other expressions
    that need to know array details (e.g., models).

    Note: the basic "array" class handles dimensions and indices,
    but does NOT store any data.
    Arrays plus data are represented by a hidden, derived class.

*/

#include "expr.h"
#include "result.h"
#include "symbols.h"
#include "iterators.h"

// ******************************************************************
// *                                                                *
// *                        array_item class                        *
// *                                                                *
// ******************************************************************

/**  The actual items stored in arrays.
    If the expression is non-null, it should be used;
    otherwise, take the result.
*/
class array_item : public shared_object {
    public:
        result r;
        expr* e;
    public:
        array_item(expr* rhs);
    protected:
        virtual ~array_item();
    public:
        virtual bool Print(std::ostream &s, int w=0) const;
        inline void Compute(traverse_data &x, bool subst) {
            if (0==e) {
                (*x.answer) = r;
                return;
            }
            e->Compute(x);
            if (subst) {
                r = *x.answer;
                Delete(e);
                e = 0;
            }
        }
};

// ******************************************************************
// *                                                                *
// *                          array  class                          *
// *                                                                *
// ******************************************************************

/**   The base class of arrays.

      Note that this handles dimensions and indices,
      but does NOT store any data.
*/

class array : public symbol {
    public:
        array(const array* wrapper);
        array(const location &W, const type* t, char* n, iterator** il, int dim);
    protected:
        virtual ~array();

    public:
        inline int GetDimension() const { return dimension; }

        inline const type* GetIndexType(int i) const {
            CHECK_RANGE(0, i, dimension);
            DCASSERT(index_list);
            DCASSERT(index_list[i]);
            return index_list[i]->Type();
        }

        inline const char* GetIndexName(int i) const {
            CHECK_RANGE(0, i, dimension);
            DCASSERT(index_list);
            DCASSERT(index_list[i]);
            return index_list[i]->Name();
        }

        inline bool IsFixed() const { return is_fixed; }

        /** For the current values of the iterators,
            set the return "value" of the array.
            @param  retval  Return value to assign.
            @param  rename  If true, we will give the expression
                            \a retval an appropriate name.
                            Do not set this to true unless
                            \a retval is a symbol.
        */
        virtual void SetCurrentReturn(expr* retval, bool rename);

        /** For the current values of the iterators,
            obtain the return value function.
            This should NOT be deleted or Deleted!
            @return 0, if the return value has not been set yet;
                        the current return value, otherwise.
        */
        virtual array_item* GetCurrentReturn();

        /** Obtain the array "value" for the given indices.
            Should be used for "calling" the array.
                @param  indexes   List of Indexes to use,
                                  should have length equal to the "dimension".

                @param  x         Used to propogate any index errors.

                @return   0, if any of the indexes were out of range.
                             The return value, otherwise.
        */
        virtual array_item* GetItem(expr** indexes, result &x);

        /** Checks an array call, promoting indexes as necessary.
                @param  fn        Filename of call.
                @param  ln        Line number of call.
                @param  indexes   Passed indexes.
                @param  dim       Number of passed indexes.
                @return true,   iff the indexes were successfully promoted
                                to match the expected type and number.
                        false,  otherwise (will make noise).
        */
        bool checkArrayCall(const location &W, expr** indexes, int dim) const;

        virtual void Traverse(traverse_data &x);
        void PrintHeader(std::ostream &s) const;

        //
        // Statics to build and type check arrays
        //

        /** Make a new array.

            @param  W     Where defined.
            @param  t       Type of the array.
            @param  name    Name of the array.
            @param  indexes List of iterators, which define the "shape"
                            of the array.  Each iterator must have been
                            created by calling MakeIterator().
            @param  dim     Length of the list of indexes.
                            Can be considered the dimension of the array.
            @return 0,  if some error occurred (will make noise).
                        A new array, otherwise.
        */
        static symbol* makeArray(const location& W, const type* t,
            char* n, symbol** indexes, int dim);

        /** Make an array assignment statement, not within a converge block.
            These handle statements of the form
                int a[i][j][k] := rhs;

            @param  W       Where defined.
            @param  array   Array to use for the assignment.
                            Must have been created by a call to MakeArray().
            @param  rhs     The right-hand side of the assignment.
            @return ERROR,  if some error occurred (will make noise).
                            A new statement, otherwise.
        */
        static expr* makeArrayAssign(const location& W, symbol* array,
            expr* rhs);

        /** Make an array dereferencing expression.
            These handle expressions of the form
                a[3][5+n][4-3*foobar(7, x)]

            @param  W     Where defined.
            @param  array   Array to use.
                            Must have been created by a call to MakeArray().
            @param  indexes List of indices to be "passed" to the array.
            @param  dim     Number of indices in the list \a indexes.
            @return ERROR,  if some error occurred (will make noise).
                            A new expression, otherwise.
        */
        static expr* makeArrayCall(const location& W, symbol* array,
            expr** indexes, int dim);

    protected:
        array* instantiateMe() const;

    protected:
        /// Info about enclosing for iterators.
        iterator** index_list;
        /// The number of iterators.
        int dimension;
        /// Can we remove the expressions in the array?
        bool is_fixed;
};

#endif
