
#ifndef TYPE_H
#define TYPE_H

#include "../include/defines.h"
#include "../Utils/strings.h"
#include <string.h>

class result;
class splayOfShared;

// class io_environ;

// all of this required for infinity string option.
// class option;
// class option_manager;
// class exprman;

typedef unsigned char  modifier;

const modifier  DETERM = 0;
const modifier  PHASE = 1;
const modifier  RAND  = 2;
const modifier  ANY_MODIFIER = 254;
const modifier  NO_SUCH_MODIFIER = 255;

// ******************************************************************
// *                                                                *
// *                           type class                           *
// *                                                                *
// ******************************************************************

class simple_type;

/** New type mechanism.
    This is an abstract base class.
    Since we only have a few types, this should make life significantly
    easier without introducing too much overhead.

    It's a shared object, so it can go in splay trees :)
*/
class type : public shared_string {
    public:
        static const type* null;

    public:
        type(const char* n);
        type(const std::string &s);
    public:
        // Inherits Print and Compare from shared_string.
        bool matches(const char* n) const;
        static inline bool matches(const type* t, const char* n) {
            return t ? t->matches(n) : false;
        }

        inline void NoFunctions() { func_definable = false; }
        inline void NoVariables() { var_definable = false; }

        inline bool canDefineFuncOfThis() const { return func_definable; }
        inline bool canDefineVarOfThis() const { return var_definable; }

        inline bool isVoid() const { return is_void; }
        inline bool isAFormalism() const { return is_formalism; }

        virtual const type* getSetElemType() const;
        inline bool isASet() const { return getSetElemType(); }
        virtual const type* getSetOfThis() const;

        virtual modifier getModifier() const;
        virtual const type* modifyType(modifier m) const;
        virtual const type* removeModif() const;

        virtual bool hasProc() const;
        virtual const type* removeProc() const;
        virtual const type* addProc() const;
        virtual void setProc(const type* t);

        /// Strips all set, proc, modifiers.
        virtual const simple_type* getBaseType() const = 0;

        /// Neat trick: change the base type, keep set, proc, modifier status.
        virtual const type* changeBaseType(const type* newbase) const;

        /** Comparison, for purposes of maintaining sets of this type.
            Default behavior is to throw an error.
            Works like "strcmp".
            Any total ordering can be used for elements of the type,
            even an ordering different from operators "<", ">", etc.
            But it must be transitive:
                compare(a,b)>0 and compare(b,c)>0 implies compare(a,c)>0
            and it must be the case that
                compare(a,b)==0 iff a==b

            @param  a  First item.
            @param  b  second item.
            @return positive,   if a is larger than b,
                    zero,       if a equals b,
                    negative,   if a is less than b.
        */
        virtual int compare(const result& a, const result& b) const;


        /// Can we print objects of this type.
        inline bool isPrintable() const { return printable; }
        inline void setPrintable() { printable = true; }

        /** Print an abnormal result.
            These are the same output regardless of type.
                @param  s   Stream to write to
                @param  r   Result to display, must not be Normal().
                @param  w   Width to use (defaults to 0)
        */
        static bool print_abnormal(std::ostream &s, const result& r, int w=0);

        /** Print a result of this type.
            We must be "printable" according to isPrintable().
                @param  s  Stream to write to.
                @param  r  Result to display.
                @param  w  Width to use (defaults to 0).
                @param  p  Precision to use (negative for default).
        */
        bool print(std::ostream &s, const result& r, int w=0, int p=-1) const;

        /** Show a result.
            Just like print(), except for strings:
            show() will give you "the string.\n",
            while print() will give you the string.
        */
        void show(std::ostream &s, const result& r) const;

        /** Fill the result, from a string.
            Works only for "simple" types.
            If the desired type is "STRING", then we simply
            copy the string.
            If the desired type is "INT" or "REAL",
            then the following special strings are recognized,
            in addition to the usual numerical ones.
                infinity  : for positive infinity
                -infinity  : for negative infinity
                ?    : for "don't know"
            On return, the result will hold the appropriate value,
            or null if the conversion was not possible.
                @param  r  Where the result will be stored.
                @param  s  Input string.
                @return true on success.
        */
        virtual void assignFromString(result& r, const char* s) const;


        //
        // Type system static methods
        //

        /// Returns the modifier with given name, or NO_SUCH_MODIF.
        static modifier findModifier(const char* name);

        /**
            Register a type into the type system.
                @param  t   New type to add.
                @return     If there's already a type with the same name as
                            t, return the existing type; otherwise return t.
         */
        static simple_type* registerNew(simple_type* t);

        /**
            Find a simple type.
            These are single-word type names with no modifiers.
                @param  tname   type name
                @return     A pointer to the matching type, or null.
         */
        static simple_type*  find(const char* tname);

        /**
            Find a modified type.
            For convenience; you could do this by hand.
                @param  set     If true, build a set
                @param  proc    If true, add proc to the type
                @param  mod     Modifier; use DETERM for none
                @param  tname   Simple type name
                @return     The desired type, or null if the base type
                            or any modifications are not possible.
        */
        static const type* find(bool set, bool proc, modifier mod,
                        const char* tname);

        /**
            Build "proc t" as a valid type.
                @param  t   Base (simple) type.  For modified types,
                            use allowProcMod instead.
        */
        static void allowProc(simple_type* t);

        /**
            Build "mod t", and maybe "proc mod t", as valid types.
                @param  proc    If true, also add proc mod t.
                @param  mod     Modifier (PH or RAND; all others ignored).
                @param  t       Base (simple) type.
        */
        static void allowProcMod(bool proc, modifier mod, simple_type* t);

        /**
            Build "{t}" as a valid type (set with elements of type t).
                @param  t   Base (simple) type.
        */
        static void allowSetsOf(simple_type* t);

        //
        // TBD: might want to redesign these methods used for documentation
        //
        static unsigned numRegistered();
        static const simple_type* getRegistered(unsigned i);

    protected:
        inline void setVoid()               { is_void = true; }
        inline void setFormalism()          { is_formalism = true; }

        virtual bool print_normal(std::ostream &s, const result& r,
                int w=0, int p=-1) const;
        virtual void show_normal(std::ostream &s, const result& r) const;
        virtual void assign_normal(result& r, const char* s) const;
        virtual int compare_normal(const result &x, const result &y) const;

    protected:
        static const unsigned GENERAL    = 0;
        static const unsigned FIXED      = 1;
        static const unsigned SCIENTIFIC = 2;

        static unsigned real_format;    // format for reals
        static const char* int_comma;   // thousands sep for ints
        static const char* real_comma;  // thousands sep for reals
        static const char* pos_infinity_string;
        static const char* neg_infinity_string;

        friend class type_initializer;

    private:
        void init();

    private:
        bool is_void;
        bool func_definable;
        bool var_definable;
        bool printable;
        bool is_formalism;

        static splayOfShared* allSimple;
};

// ******************************************************************
// *                                                                *
// *                         typelist class                         *
// *                                                                *
// ******************************************************************

/** Handy class for aggregate types.
    And it allows us to "share" them :^)
*/
class typelist : public shared_object {
        const type** list;
        unsigned nt;
    public:
        typelist(unsigned nt);
        virtual ~typelist();
        inline void SetItem(unsigned n, const type* t) {
            DCASSERT(list);
            DCASSERT(n < nt);
            DCASSERT(t);
            list[n] = t;
        }
        inline const type* GetItem(unsigned n) const {
            DCASSERT(list);
            DCASSERT(n < nt);
            return list[n];
        }
        inline unsigned Length() const {
            return nt;
        }
        // Required for shared_object
        virtual bool Print(std::ostream &s, int w=0) const;
        virtual int Compare(const shared_object *o) const;
};


// ******************************************************************
// *                                                                *
// *                       simple_type  class                       *
// *                                                                *
// ******************************************************************

/** Simple type with value, such as integer or boolean.
*/
class simple_type : public type {
        const type* phase_this;
        const type* rand_this;
        const type* proc_this;
        const type* set_this;
    protected:
        const char* short_docs;
        const char* long_docs;
    public:
        simple_type(const char* n, const char* sd, const char* ld);
        virtual ~simple_type();

        // Documentation
        inline const char* shortDocs() const { return short_docs; }
        inline const char* longDocs() const { return long_docs; }


        inline void setPhase(const type* t) {
            DCASSERT(!phase_this);
            phase_this = t;
        }
        inline void setRand(const type* t) {
            DCASSERT(!rand_this);
            rand_this = t;
        }
        inline void setSet(const type* t) {
            DCASSERT(!set_this);
            set_this = t;
        }

        virtual const type* getSetOfThis() const;
        virtual const type* modifyType(modifier m) const;
        virtual const type* addProc() const;
        virtual void setProc(const type* t);

        virtual const simple_type* getBaseType() const;
        virtual const type* changeBaseType(const type* newbase) const;
};


// ******************************************************************
// *                                                                *
// *                        void_type  class                        *
// *                                                                *
// ******************************************************************

/** void types with no value, such as void, "state" or "place".
*/
class void_type : public simple_type {
    public:
        void_type(const char* n, const char* sd, const char* ld);

        /// Default comparison: check addresses of "other" field pointers.
        virtual int compare(const result& a, const result& b) const;
};

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

inline std::ostream& operator << (std::ostream &s, const type &t)
{
    t.Print(s);
    return s;
}

/// Safe way to get a modifier
inline modifier GetModifier(const type* t)
{
    if (t)  return t->getModifier();
    return NO_SUCH_MODIFIER;
}

inline bool HasProc(const type* t)
{
    if (t)  return t->hasProc();
    return false;
}

inline const simple_type* GetBase(const type* t)
{
    if (t)  return t->getBaseType();
    return nullptr;
}

inline const type* ModifyType(modifier m, const type* t)
{
    if (t)  return t->modifyType(m);
    return nullptr;
}

inline const type* ProcifyType(const type* t)
{
    if (t)  return t->addProc();
    return nullptr;
}

inline const type* ApplyPM(const type* pm, const type* bt)
{
    if (!pm)  return bt;
    return pm->changeBaseType(bt);
}

/// Handy routine to go from x ph y -> x rand y.
inline const type* Phase2Rand(const type* lct)
{
    if (!lct) return nullptr;
    if (lct->getModifier() != PHASE) return lct;
    const type* ans = lct->getBaseType();
    DCASSERT(ans);
    ans = ans->modifyType(RAND);
    DCASSERT(ans);
    if (lct->hasProc()) ans = ans->addProc();
    return ans;
}



#endif
