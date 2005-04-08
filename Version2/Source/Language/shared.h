
// $Id$

#ifndef SHARED_H
#define SHARED_H

/** @name shared.h
    @type File
    @args \ 

  Base class for shared objects.
  I.e., any time we want to make a shallow copy, and be able to
  safely delete objects, derive that class from here.

  Used for string results and (eventually) other large objects;
  also, expressions.
*/

#include "../defines.h"

//@{


// ******************************************************************
// *                                                                *
// *                      shared_object  class                      *
// *                                                                *
// ******************************************************************

/**   Abstract base class for sharing pointers to objects.
 
*/  

class shared_object {
  int linkcount;
public:
  shared_object() { linkcount = 1; }
  virtual ~shared_object() { }
  inline bool isDeleted() const { return linkcount<1; }
  inline int numRefs() const { return linkcount; }
  friend shared_object* Share(shared_object *o);
  friend void Delete(shared_object* o);
};

/**  Create a shallow copy of this object.
     Basically, like creating a hard link to a file.
 */
inline shared_object* Share(shared_object *o)
{
  if (NULL==o) return NULL;
  DCASSERT(o->linkcount>0);
  o->linkcount++; 
  return o;
}

/**  Delete a shared object.
     This should *always* be called instead of doing it "by hand", 
     because the object might be shared.  This version takes sharing 
     into account.
 */
inline void Delete(shared_object* o)
{
  if (NULL==o) return;
  DCASSERT(o->linkcount>0);
  o->linkcount--;
  if (0==o->linkcount) {
    delete o;
  }
}

//@}

#endif

