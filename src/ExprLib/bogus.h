
#ifndef BOGUS_H
#define BOGUS_H

#include "expr.h"

// ******************************************************************
// *                        bogus_expr class                        *
// ******************************************************************

/** Bogus expressions.
    Used for the special, compile-time expressions "error" and "default".
*/
class bogus_expr : public expr {
  const char* which;
public:
  bogus_expr(const char* w);
  virtual bool Print(std::ostream &s, int width) const;
  virtual int Compare(const shared_object* o) const;
  virtual void PrintType(std::ostream &s) const;
protected:
  virtual void Traverse(traverse_data &x);
};

#endif

