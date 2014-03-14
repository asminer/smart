
// $Id$

/** \file biginttype.h

    Module for arbirary-precision integers.
    Defines a bigint type, and appropriate operators.
*/

#ifndef BIGINTTYPE_H
#define BIGINTTYPE_H

class exprman;
class symbol_table;

#include "config.h"
#ifdef HAVE_LIBGMP
#include <gmp.h>
#endif

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
#ifdef HAVE_LIBGMP
  mpz_t value;
#else
  long value;
#endif
  bigint();
  bigint(long x);
  bigint(const char* c);
#ifdef HAVE_LIBGMP
  bigint(const mpz_t &v);
#endif
  bigint(const bigint &b);
  virtual ~bigint();
  virtual bool Print(OutputStream &s, int width) const;
  virtual bool Equals(const shared_object *o) const;
  
  // Handy stuff:

  inline void set_si(long i) {
#ifdef HAVE_LIBGMP
    mpz_set_si(value, i);
#else
    value = i;
#endif
  }

  inline void add_ui(unsigned long i) {
#ifdef HAVE_LIBGMP
    mpz_add_ui(value, value, i);
#else
    value += i;
#endif
  }
  inline void add(const bigint &a, const bigint &b) {
#ifdef HAVE_LIBGMP
    mpz_add(value, a.value, b.value);
#else
    value = a.value + b.value;
#endif
  }

  inline void sub(const bigint &a, const bigint &b) {
#ifdef HAVE_LIBGMP
    mpz_sub(value, a.value, b.value);
#else
    value = a.value - b.value;
#endif
  }

  inline void neg(const bigint &b) {
#ifdef HAVE_LIBGMP
    mpz_neg(value, b.value);
#else
    value = -b.value;
#endif
  }

  inline void mul(const bigint &a, const bigint &b) {
#ifdef HAVE_LIBGMP
    mpz_mul(value, a.value, b.value);
#else
    value = a.value * b.value;
#endif
  }

  inline void div_r(const bigint &a, const bigint &b) {
#ifdef HAVE_LIBGMP
    mpz_tdiv_r(value, a.value, b.value);
#else
    value = a.value % b.value;
#endif
  }

  inline void div_q(const bigint &a, const bigint &b) {
#ifdef HAVE_LIBGMP
    mpz_tdiv_q(value, a.value, b.value);
#else
    value = a.value / b.value;
#endif
  }

  inline int cmp_si(long i) const {
#ifdef HAVE_LIBGMP
    return mpz_cmp_si(value, i);
#else
    return value - i;
#endif
  }

  inline int cmp(const bigint &b) const {
#ifdef HAVE_LIBGMP
    return mpz_cmp(value, b.value);
#else
    return value - b.value;
#endif
  }

  inline bool fits_slong() const {
#ifdef HAVE_LIBGMP
    return mpz_fits_slong_p(value);
#else
    return true;
#endif
  }

  inline long get_si() const {
#ifdef HAVE_LIBGMP
    return mpz_get_si(value);
#else
    return value;
#endif
  }
};

/** Initialize bigint module.
    Nice, minimalist front-end.
      @param  em  The expression manager to use.
      @param  st  Symbol table to add any bigint functions.
                  If 0, functions will not be added.
*/
void InitBigintType(exprman* em, symbol_table* st);

#endif
