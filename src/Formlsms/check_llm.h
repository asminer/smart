
// $Id$

#ifndef CHECK_LLM_H
#define CHECK_LLM_H

#include "../ExprLib/mod_inst.h"

class intset;
class stateset;

// ******************************************************************
// *                                                                *
// *                     checkable_lldsm  class                     *
// *                                                                *
// ******************************************************************

/**   The base class models that are "checkable" (think CTL)
*/  
class checkable_lldsm : public graph_lldsm {
public:
  checkable_lldsm(model_type t);

  /** Get the reachable states, as a stateset.
      Default behavior here is to (quietly) set the result to null.
        @param  ss  Set of reachable states is stored here,
                    as a "stateset".
  */
  virtual void getReachable(result &ss) const;

  /** Get the possible initial (time 0) states.
      Conceptually, this tells which elements in the
      vector constructed by getInitialDistribution() 
      will have non-zero probability.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  x   On input: ignored.
                    On output: an appropriate "stateset"
                    containing the set of states the model
                    could be in at time 0.
                    Will be a "null" result on error.
  */
  virtual void getInitialStates(result &x) const;

  /** Get the (potential) states that, once entered,
      are never left.  This includes deadlocked states.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  x   On input: ignored.
                    On output: an appropriate "stateset"
                    containing the set of possible
                    absorbing states for the model.
                    Will be a "null" result on error.
  */
  virtual void getAbsorbingStates(result &x) const;

  /** Get the (potential) deadlocked states.
      That means states that have no outgoing edges
      (even to itself).
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  x   On input: ignored.
                    On output: an appropriate "stateset"
                    containing the set of possible
                    deadlocked states for the model.
                    Will be a "null" result on error.
  */
  virtual void getDeadlockedStates(result &x) const;

  /** Get the set of states satisfying a constraint.
      Default behavior here is to (quietly) set the result to null.
        @param  p   Logical condition for states to satisfy.
        @param  ss  Set of "potential" states satisfying p is stored here,
                    as a "stateset".
  */
  virtual void getPotential(expr* p, result &ss) const;

  /** For CTL model checking, is this a "fair" model?
      This says that infinite paths that are based on
      making one particular choice infinitely often
      are not considered (e.g., in a Markov chain,
      where such a path has probability zero).
      Default is to return false.
        @return true  If the model is fair, and CTL algorithms
                      must take this into account.
                false otherwise.
  */
  virtual bool isFairModel() const;

  /** Determine TSCCs satisfying a property.
      This is done "in place".
      Should only be called for "fair" models.
      Default behavior is to print an error message.
         @param  p  On input: property p.
                    On output: states are removed if
                    they are not in a TSCC, or if
                    not all states in the TSCC satisfy p.
  */
  virtual void getTSCCsSatisfying(stateset &p) const;

  /** Find all "deadlocked" states.
      Default behavior is to print an error message.
        @param  ss  If i is not a deadlocked state,
                    then i will be removed from ss.
  */
  virtual void findDeadlockedStates(stateset &ss) const;

  /** Get states reachable from us in one step.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  p     Set of source states.
        @param  r     On output, we add any state that can be reached 
                      from a state in p, in one "step".
                      Note: if p and r are the same, then we might
                      add states that are more than one "step" away.
        @return true  If any states were added to r,
                false otherwise.
  */
  virtual bool forward(const intset &p, intset &r) const;

  /** Get states that reach us in one step.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  p     Set of target states.
        @param  r     On output, we add any state that can reach
                      a state in p, in one "step".
                      Note: if p and r are the same, then we might
                      add states that are more than one "step" away.
        @return true  If any states were added to r,
                false otherwise.
  */
  virtual bool backward(const intset &p, intset &r) const;


  // Hacks for explicit:

  /** Is the given state "absorbing".
      This means that either there are no outgoing edges from this state,
      OR there is a single outgoing edge, to itself.
      Default behavior is to print an error message.
        @param  st    State (index) we are interested in.
        @return true, iff it is impossible to leave state st.
  */
  virtual bool isAbsorbing(long st) const;

  /** Is the given state "deadlocked".
      This means that there are no outgoing edges.
      Default behavior is to print an error message.
        @param  st    State (index) we are interested in.
        @return true, iff there are no outgoing edges from state st.
  */
  virtual bool isDeadlocked(long st) const;

};

#endif

