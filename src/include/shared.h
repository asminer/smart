
#ifndef SHARED_H
#define SHARED_H

#include "defines.h"

// #define DEBUG_LINKCOUNTS
// #define DISPLAY_LINKCOUNTS

class OutputStream;

#ifdef DEBUG_LINKCOUNTS
  #include "streams.h"
#endif
#ifdef DISPLAY_LINKCOUNTS
  #include "streams.h"
#endif

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
  long linkcount;
public:
  shared_object() {
    linkcount = 1;
  }
protected:
  virtual ~shared_object() {
  }
public:
  inline long numRefs() const { 
    return linkcount; 
  }
  /// Safer to call template function Share() below.
  inline void ShareMe() {
#ifdef DEBUG_LINKCOUNTS
    if (linkcount <= 0) {
      DisplayStream cout(stderr);
      cout << "Sharing a deleted object: ";
      Print(cout, 0);
      cout << "\n";
    }
#endif
    DCASSERT(linkcount > 0);
    linkcount++;
  }
  /** Write the object to the given stream.
        @param  s     The output stream to write to.
        @param  width Number of slots to use.
                      This is only allowed for certain objects.
                      If zero, consume exactly the amount of
                      space required.
                      If positive, add spaces before the object
                      so that \a width space is consumed.
                      If negative, add spaces after the object
                      so that \a -width space is consumed.

        @return  true  if anything was printed, false otherwise.

  */
  virtual bool Print(OutputStream &s, int width) const = 0;
  virtual bool Equals(const shared_object *o) const = 0;
  friend void Delete(shared_object* o);
};

/** Create a shallow copy of this object.
    Basically, like creating a hard link to a file.
*/
template <class SHARED>
inline SHARED* Share(SHARED *o)
{
  if (0==o) return 0;
  o->ShareMe();
#ifdef DISPLAY_LINKCOUNTS
  DisplayStream cout(stderr);
  cout << "+1 (total " << o->numRefs() << ") for object: ";
  o->Print(cout, 0);
  cout << "\n";
#endif
  return o;
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
  if (o->linkcount <= 0) {
    DisplayStream cout(stderr);
    cout << "Too many deletes for object: ";
    o->Print(cout, 0);
    cout << "\n";
  }
#endif
  DCASSERT(o->linkcount>0);
  o->linkcount--;
#ifdef DISPLAY_LINKCOUNTS
  DisplayStream cout(stderr);
  cout << "-1 (total " << o->numRefs() << ") for object: ";
  o->Print(cout, 0);
  cout << "\n";
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

#endif
