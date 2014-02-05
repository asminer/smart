
// $Id$

/** \file biginttype.h

    Module for arbirary-precision integers.
    Defines a bigint type, and appropriate operators.
*/

#ifndef BIGINTTYPE_H
#define BIGINTTYPE_H

class exprman;
class symbol_table;

#include <gmp.h>

// ******************************************************************
// *                                                                *
// *                          bigint class                          *
// *                                                                *
// ******************************************************************

/// Very thin wrapper around GMP large integers.
class bigint : public shared_object {
private:
  static char* buffer;
  static int bufsize;
public:
  mpz_t value;
  bigint();
  bigint(long x);
  bigint(const char* c);
  bigint(const mpz_t &v);
  bigint(const bigint &b);
  virtual ~bigint();
  virtual bool Print(OutputStream &s, int width) const;
  virtual bool Equals(const shared_object *o) const;
  
  // Handy stuff:
};

/** Initialize bigint module.
    Nice, minimalist front-end.
      @param  em  The expression manager to use.
      @param  st  Symbol table to add any bigint functions.
                  If 0, functions will not be added.
*/
void InitBigintType(exprman* em, symbol_table* st);

#endif
