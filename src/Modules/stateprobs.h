
// $Id$

/** \file stateprobs.h

    Module for stateprobs.
    Defines a stateprob type.
*/

#ifndef STATEPROBS_H
#define STATEPROBS_H

class exprman;
class symbol_table;
class stochastic_lldsm;

// ******************************************************************
// *                                                                *
// *                        stateprobs class                        *
// *                                                                *
// ******************************************************************

/// Explicit only (at least for now)
class stateprobs : public shared_object {
  const stochastic_lldsm* parent;
  double* dist;
  long numStates;
public:
  stateprobs(const stochastic_lldsm* p, double* d, long N);
  virtual ~stateprobs();
public:
  inline const stochastic_lldsm* getParent() const { return parent; }

  // required for shared_object
  virtual bool Print(OutputStream &s, int width) const;
  virtual bool Equals(const shared_object *o) const;
private:
  static bool print_indexes;
  friend void InitStateprobs(exprman* em, symbol_table* st);
};


/** Initialize stateprobs module.
    Nice, minimalist front-end.
      @param  em  The expression manager to use.
      @param  st  Symbol table to add any stateprobs functions.
                  If 0, functions will not be added.
*/
void InitStateprobs(exprman* em, symbol_table* st);

#endif
