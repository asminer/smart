
#ifndef SHARED_H
#define SHARED_H

#include "defines.h"
#include <iostream>

// #define DEBUG_LINKCOUNTS
// #define DISPLAY_LINKCOUNTS

// ******************************************************************
// *                                                                *
// *                      shared_object  class                      *
// *                                                                *
// ******************************************************************

/** Abstract base class for sharing pointers to objects.

    In particular, this is used for all computed results
    of expressions, except for the basic types of BOOL,
    INT, and REAL (for speed).
*/
class shared_object {
    // TBD: make this unsigned long?
    long linkcount;
public:
    shared_object() {
        linkcount = 1;
    }
protected:
    virtual ~shared_object() { }
public:
    inline long numRefs() const {
        return linkcount;
    }

    /** Write the object to the given stream.
            @param  s       The output stream to write to.
            @param  width   Number of slots to use.
                            This is only allowed for certain objects.
                            If zero, consume exactly the amount of
                            space required.
                            If positive, add spaces before the object
                            so that \a width space is consumed.
                            If negative, add spaces after the object
                            so that \a -width space is consumed.

    */
    virtual bool Print(std::ostream &s, int width=0) const = 0;

    /** Compare with ourself.
        If objects will never be compared, it is fine
        to have this method do nothing or abort.
            @return 0, iff o equals this object;
                    negative, if o succeeds this object;
                    positive, if o preceeds this object.
    */
    virtual int Compare(const shared_object* o) const {
        return this - o;
    }

protected:
    inline shared_object* ShareMe() {
#ifdef DEBUG_LINKCOUNTS
        if (linkcount < 1) {
            std::cerr << "Sharing a deleted object: ";
            Print(std::cerr, 0);
            std::cerr << endl;
        }
#endif
        DCASSERT(linkcount > 0);
        linkcount++;
#ifdef DISPLAY_LINKCOUNTS
        std::cerr << "+1 (total " << linkcount << ") for object: ";
        Print(std::cerr, 0);
#endif
        return this;
    }

    friend shared_object* _Share(shared_object* o);
    friend void Delete(shared_object* o);
};

inline std::ostream& operator<< (std::ostream& s, const shared_object &o)
{
    o.Print(s);
    return s;
}

/** Create a shallow copy of this object.
    Basically, like creating a hard link to a file.
*/
inline shared_object* _Share(shared_object* o)
{
    return o ? o->ShareMe() : 0;
}

/**
 * Templated version of Share to get the same type of pointer out.
 */
template <class SHARED>
inline SHARED* Share(SHARED *o)
{
    return static_cast <SHARED*> (_Share(o));
}

/** Delete a shared object.
    This should *always* be called instead of doing it "by hand",
    because the object might be shared.  This version takes sharing
    into account.
*/
inline void Delete(shared_object* o)
{
    if (0==o) return;
#ifdef DEBUG_LINKCOUNTS
    if (o->linkcount < 1) {
        std::cerr << "Too many deletes for object: ";
        o->Print(std::cerr, 0);
        std::cerr << endl;
    }
#endif
    DCASSERT(o->linkcount>0);
    o->linkcount--;
#ifdef DISPLAY_LINKCOUNTS
    std::cerr << "-1 (total " << o->linkcount << ") for object: ";
    o->Print(std::cerr, 0);
    std::cerr << endl;
#endif
    if (0==o->linkcount) {
#ifndef DEBUG_LINKCOUNTS
        delete o;
#endif
    }
}

/// Handy way to delete and set to null
template <class SHARED>
inline void Nullify(SHARED* &ptr)
{
    Delete(ptr);
    ptr = 0;
}

// ******************************************************************
// *                                                                *
// *                      shared_updater class                      *
// *                                                                *
// ******************************************************************

class shared_updater {
    public:
        virtual void update(shared_object* obj) = 0;
};

// ******************************************************************
// *                                                                *
// *                      shared_visitor class                      *
// *                                                                *
// ******************************************************************

class shared_visitor {
    public:
        virtual void visit(const shared_object* obj) = 0;
};


#endif
