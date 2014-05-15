
// $Id$

/** \file statevects.h

    Module for statevects.
    Defines the statedist, stateprobs, statemsrs types.
*/

#ifndef STATEPROBS_H
#define STATEPROBS_H

class exprman;
class symbol_table;
class stochastic_lldsm;

// ******************************************************************
// *                                                                *
// *                        statevect  class                        *
// *                                                                *
// ******************************************************************

/// Explicit only (at least for now)
class statevect : public shared_object {
  const stochastic_lldsm* parent;
  double* vect;
  long numStates;
public:
  statevect(const stochastic_lldsm* p, double* d, long N);
  virtual ~statevect();
public:
  inline const stochastic_lldsm* getParent() const { return parent; }

  // required for shared_object
  virtual bool Print(OutputStream &s, int width) const;
  virtual bool Equals(const shared_object *o) const;
private:
  static int display_style;
  static const int FULL = 0;
  static const int SINDEX = 1;
  static const int SSTATE = 2;
  friend void InitStatevects(exprman* em, symbol_table* st);
};


// ******************************************************************
// *                                                                *
// *                        statedist  class                        *
// *                                                                *
// ******************************************************************

class statedist : public statevect {
  public:
    statedist(const stochastic_lldsm *p, double *d, long N);

  // ANY difference?
};


// ******************************************************************
// *                                                                *
// *                        stateprobs class                        *
// *                                                                *
// ******************************************************************

class stateprobs : public statevect {
  public:
    stateprobs(const stochastic_lldsm *p, double *d, long N);

  // ANY difference?
};


// ******************************************************************
// *                                                                *
// *                        statemsrs  class                        *
// *                                                                *
// ******************************************************************

class statemsrs : public statevect {
  public:
    statemsrs(const stochastic_lldsm *p, double *d, long N);

  // ANY difference?
};


// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

/** Initialize statevects module.
    Nice, minimalist front-end.
      @param  em  The expression manager to use.
      @param  st  Symbol table to add any statevects functions.
                  If 0, functions will not be added.
*/
void InitStatevects(exprman* em, symbol_table* st);

#endif
