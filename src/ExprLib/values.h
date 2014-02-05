
// $Id$

#ifndef VALUES_H
#define VALUES_H

#include "expr.h"
#include "result.h"

// ******************************************************************
// *                                                                *
// *                          value  class                          *
// *                                                                *
// ******************************************************************

/**  The base class for values in expressions.
     That means things like "3.2", "infinity".
*/
class value : public expr {
  result val;
public:
  value(const char* fn, int line, const type* t, const result &x);
protected:
  virtual ~value();
public:
  virtual bool Print(OutputStream &s, int width) const;
  virtual bool Equals(const shared_object* o) const;
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
};

#endif
