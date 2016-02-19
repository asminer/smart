
// $Id$

#ifndef PHASE_HLM_H
#define PHASE_HLM_H

#include "../ExprLib/mod_inst.h"
#include "rng.h"
#include "stoch_llm.h"
#include <math.h>

class statedist;
class shared_state;

// ******************************************************************
// *                                                                *
// *                   Phase-type  Distributions                    *
// *                                                                *
// ******************************************************************

inline void generateBernoulli(double p, traverse_data &x)
{
  DCASSERT(x.stream);
  DCASSERT(x.answer);
  x.answer->setBool( x.stream->Uniform32() < p );
}

inline void generateGeometric(double p, traverse_data &x)
{
  DCASSERT(x.answer);
  if (0.0 == p) {
    x.answer->setInt(0);
    return;
  }
  if (1.0 == p) {
    x.answer->setInfinity(1);
    return;
  }
  DCASSERT(x.stream);
  x.answer->setInt(long(log(x.stream->Uniform32()) / log(p)));
}

inline void generateEquilikely(long a, long b, traverse_data &x)
{
  DCASSERT(x.answer);
  if (a < b) {
    DCASSERT(x.stream);
    x.answer->setInt(long(a + (b-a+1)*x.stream->Uniform32()));
    return;
  }
  if (a==b) {
    x.answer->setInt(a);
    return;
  }
  // must have a > b
  x.answer->setNull();
}

inline void generateBinomial(long n, double p, traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(x.stream);
  long count = 0;
  for ( ; n; n--) {
    if (x.stream->Uniform32() < p) count++;
  }
  x.answer->setInt(count);
}

inline void generateExpo(double lambda, traverse_data &x)
{
  DCASSERT(x.answer);
  if (lambda > 0) {
    DCASSERT(x.stream);
    x.answer->setReal( -log(x.stream->Uniform32()) / lambda );
    return;
  }
  if (0.0 == lambda) {
    x.answer->setInfinity(1);
    return;
  } 
  x.answer->setNull();
}

inline void generateErlang(long n, double lambda, traverse_data &x)
{
  DCASSERT(x.answer);
  if (lambda > 0) {
    DCASSERT(x.stream);
    double prod = 1;
    for (long i=0; i<n; i++) {
      prod *= x.stream->Uniform32();
    }
    x.answer->setReal( -log(prod) / lambda );
    return;
  }
  // Lazy!
  generateExpo(lambda, x);
}

// ******************************************************************
// *                                                                *
// *                        phase_type class                        *
// *                                                                *
// ******************************************************************

/** Internal representation of high-level phase-type distributions.
    Note: unlike version 1 of Smart, this is not based on a matrix 
    representation, but instead provides an interface that is sufficient 
    to construct a Markov chain as appropriate.

    We assume that:
      - there is a single initial state;
      - there is a single, tangible, accepting state (absorbing);
      - there is a single, tangible, trap state (absorbing),
        even for models that don't need one (for "disabling").

    This is not a limitation because we also allow "vanishing" states 
    that do not consume time; nothing is gained anyway by allowing
    multiple absorbing accepting/trap states.

    In a nutshell, this class is used to represent distributions
    in terms of the time required to reach an absorbing 
    accepting state in a Markov chain.  
*/
class phase_hlm : public hldsm {
  bool is_discrete;
public:
  phase_hlm(bool disc);

  /// Is this a continuous distribution?
  inline bool isContinuous() const { return !is_discrete; }
  /// Is this a discrete distribution?
  inline bool isDiscrete() const { return is_discrete; }

  virtual lldsm::model_type GetProcessType() const;

  // Phase stuff

  /** Sample a value from this distribution.
        @param  x   Traversal data, as in expression computation.
  */
  virtual void Sample(traverse_data &x) = 0;

  /** Fill \a s with the initial state.
        @param  s   Place to store the state.
  */
  virtual void getInitialState(shared_state* s) const = 0;
  
  /** Fill \a s with the accepting state.
        @param  s   Place to store the state.
  */
  virtual void getAcceptingState(shared_state* s) const = 0;

  /** Fill \a s with the trap state.
        @param  s   Place to store the state.
  */
  virtual void getTrapState(shared_state* s) const = 0;
  
  /// Is \a s a vanishing (zero time) state?
  virtual bool isVanishingState(const shared_state* s) const = 0;

  /// Is \a s the accepting state?
  virtual bool isAcceptingState(const shared_state* s) const = 0;

  /// Is \a s the "trap" state?
  virtual bool isTrapState(const shared_state* s) const = 0;

  /** Set the source state for the next few operations.
      Call this prior to getOutgoingFromSource().
        @param  s   State to use.
        @return     Number of potential outgoing edges, or
                    -1 on out of memory error.
  */
  virtual long setSourceState(const shared_state* s) = 0;

  /** Get the source state.
      This is the state set using setSourceState().
      Useful for hierarchy where one submodel is to stay put.
        @param  s   Source state is put here.
  */
  virtual void getSourceState(shared_state* s) const = 0;

  /** Get outgoing rate / probability from current source state.
      Note, this should work correctly even for accepting or trap states.
        @param  e   Edge number, must be between 0 and
                    the result of setSourceState()-1.
        @param  t   Destination state (output parameter).

        @return The probability or rate.  If this is not greater than 0.0,
                then the "potential" edge was in fact not possible.
  */
  virtual double getOutgoingFromSource(long e, shared_state* t) = 0;
};

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

/** Build a phase-type model for infinity.
    @param  disc  Should this be a discrete-phase type?
                  Otherwise it will be continuous.
    @return An appropriate phase-type model.
*/
phase_hlm* makeInfinity(bool disc);

/** Build a phase-type model for zero.
    @param  disc  Should this be a discrete-phase type?
                  Otherwise it will be continuous.
    @return An appropriate phase-type model.
*/
phase_hlm* makeZero(bool disc);

/// Build a discrete phase-type model for Const(n).
phase_hlm* makeConst(long n);

/// Build a discrete phase-type model for Bernoulli(p).
phase_hlm* makeBernoulli(double p);

/// Build a discrete phase-type model for Geom(p).
phase_hlm* makeGeom(double p);

/// Build a discrete phase-type model for Equilikely(a,b).
phase_hlm* makeEquilikely(long a, long b);

/// Build a discrete phase-type model for Binomial(n,p).
phase_hlm* makeBinomial(long n, double p);

/// Build a continuous phase-type model for Erlang(n, lambda).
phase_hlm* makeErlang(long n, double lambda);



/** Build a phase-type model as a sum of other phase-type models.
      @param  opnds   Array of operands.
                      These must be either all discrete, or all continuous.
                      These will be "owned" by whatever object is created.
      @param  N       Number of operands.

      @return 0, If the operation could not be completed;
              A new phase-type model, otherwise.
*/
phase_hlm* makeSum(phase_hlm** opnds, int N);



/** Build a discrete phase-type model muliplied by a constant.
      @param  X   Discrete phase-type model.
      @param  n   Constant to multiply by.

      @return 0, if the operation could not be completed (e.g., n negative);
              A new phase-type model, otherwise.
*/
phase_hlm* makeProduct(phase_hlm* X, long n);



/** Build a continuous phase-type model muliplied by a constant.
      @param  X   Continuous phase-type model.
      @param  n   Constant to multiply by.

      @return 0, if the operation could not be completed (e.g., n negative);
              A new phase-type model, otherwise.
*/
phase_hlm* makeProduct(phase_hlm* X, double n);



/** Build a phase-type model as a choice of other phase-type models.
      @param  opnds   Array of operands.
                      These must be either all discrete, or all continuous.
                      These will be "owned" by whatever object is created.
      @param  probs   Probability for each operand.
                      Will be normalized if necessary.
      @param  N       Number of operands.

      @return 0, If the operation could not be completed;
              A new phase-type model, otherwise.
*/
phase_hlm* makeChoice(phase_hlm** opnds, double* probs, int N);



/** Build a phase-type model as the k-th smallest of the arguments.
      @param  k       Smallness index.
      @param  opnds   Array of operands.
                      These must be either all discrete, or all continuous.
                      These will be "owned" by whatever object is created.
      @param  N       Number of operands.
                      We must have 1 <= k <= N.

      @return 0, If the operation could not be completed;
              A new phase-type model, otherwise.
*/
phase_hlm* makeOrder(int k, phase_hlm** opnds, int N);


/** Build a phase-type model as the TTA of some Markov chain.
      @param  d           Is this a discrete-time model?
      @param  initial     Initial distribution to use.
      @param  accept      Set of accepting states.
      @param  trap        Set of trap states (can be 0).
      @param  mc          Markov chain model.

      @return 0, if the operation could not be completed;
              A new phase-type model, otherwise.
*/
phase_hlm* makeTTA( bool d, statedist* initial,
                    shared_object* accept, const shared_object* trap, 
                    stochastic_lldsm::process* mc);

/** Convert a continuous phase to a discrete phase,
    using uniformization on the underlying Markov chain.
      @param  cph   The continuous phase model
      @param  q     The uniformization constant.  If this is not 
                    large enough, there will be a runtime error.
                    (Sorry, no easy way to determine this ahead of time.)

      @return 0, if the operation could not be completed;
              A new phase-type model, otherwise.
*/
phase_hlm* makeUniformized(phase_hlm* cph, double q);

/** Convert a continuous phase to a discrete phase,
    using the embedded DTMC of the underlying CTMC.
      @param  cph   The continuous phase model

      @return 0, if the operation could not be completed;
              A new phase-type model, otherwise.
*/
phase_hlm* makeEmbedded(phase_hlm* cph);

#endif
