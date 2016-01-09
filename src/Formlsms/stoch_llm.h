
// $Id$

#ifndef STOCH_LLM_H
#define STOCH_LLM_H

#ifndef INITIALIZERS_ONLY

#include "graph_llm.h"

class statedist;

// ******************************************************************
// *                                                                *
// *                     stochastic_lldsm class                     *
// *                                                                *
// ******************************************************************

/**   The base class for stochastic models.

      TBD - Need to redesign this.
*/  
class stochastic_lldsm : public graph_lldsm {

// TBD - move options here

public:
    /**
    .   Stochastic processes.
        Markov chains or other fun things as derived classes.

        TBD - what goes in here?
    */
    class process : public shared_object {
        const stochastic_lldsm* parent;
      protected:
        static exprman* em;
      public:
        process();
      protected:
        virtual ~process();
        virtual const char* getClassName() const = 0;

        /**
          Hook for any desired preprocessing.
          The reachable states are also given, in case finishing the 
          reachability graph requires renumbering the states.
          Default behavior here does nothing.
        */
        virtual void Finish(state_lldsm::reachset* rss);

      public:
        inline void setParent(const stochastic_lldsm* p) {
          if (parent != p) {
            DCASSERT(0==parent);
            parent = p;
          }
        }

        inline const graph_lldsm* getParent() const {
          return parent;
        }

        inline const hldsm* getGrandParent() const {
          return parent ? parent->GetParent() : 0;
        }

        friend void InitializeStochasticLLM(exprman* em);
    };
    // ------------------------------------------------------------
    // end of inner class process

  /// probability of eventually hitting the accept state
  double accept_prob;
  /// probability of eventually hitting the trap state
  double trap_prob;
  /// mean time to absorption
  double mtta;
  /// variance of time to absorption
  double vtta;
public:
  stochastic_lldsm(model_type t);
protected:
  virtual ~stochastic_lldsm();
  virtual const char* getClassName() const { return "stochastic_lldsm"; }

public: // These methods are used for phase types.

  // TBD - can these go in the "process" inner class?

  inline void setAcceptProb(double p) { accept_prob = p; }
  inline void setTrapProb(double p)   { trap_prob = p; }
  inline void setMTTA(double v)       { mtta = v; }
  inline void setVTTA(double v)       { vtta = v; }
  inline void setInfiniteMTTA()       { mtta = -1; }
  inline void setInfiniteVTTA()       { vtta = -1; }

  inline double getAcceptProb() const { return accept_prob; }
  inline double getTrapProb() const   { return trap_prob; }
  inline double getMTTA() const       { return mtta; }
  inline double getVTTA() const       { return vtta; }
  inline bool   finiteMTTA() const    { return mtta >= 0; }
  inline bool   finiteVTTA() const    { return vtta >= 0; }

  inline bool   knownAcceptProb() const { return accept_prob >= 0; }
  inline bool   knownTrapProb() const   { return accept_prob >= 0; }
  inline bool   knownMTTA() const       { return mtta > -2; }
  inline bool   knownVTTA() const       { return vtta > -2; }

public:

  /** Get the "accepting" state.
      Currently, used only for phase types.
      Default behavior is to return -1.
        @return Index of the accepting state, if one has been designated;
                -1, otherwise.
  */
  virtual long getAcceptingState() const;

  /** Get the "trap" state.
      Currently, used only for phase types.
      Default behavior is to return -1.
        @return Index of the trap state, if one has been designated;
                -1, otherwise.
  */
  virtual long getTrapState() const;

  /** Get the number of terminal SCCs in the reachability graph.
      Useful for classifying states in a Markov chain.
      The default version provided here will only work if
      the number of TSCCs fits in a long.
        @param  count   Number of edges is stored here,
                        as a "bigint" if that type exists and there are
                        a large number of TSCCs, otherwise as a long.
  */
  virtual void getNumClasses(result& count) const;

  /** Get the number of terminal SCCs in the reachability graph.
      Useful for classifying states in a Markov chain.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  show  If true, the states in each class are displayed
                      to the Output stream, unless the function returns -1.
        @return  The number of terminal SCCs, if it fits in a long;
                -1 otherwise (on overflow).
  */
  virtual long getNumClasses(bool show) const;
     
  /** Get the states that belong to a given class.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  cl          Class number.
                            Class 0 refers to transient states.
                            The rest refer to terminal SCCs.
        @param  statelist   Set of states is stored here.
  */
  virtual void getClass(long cl, intset &statelist) const;
  

  /** Is the given state "transient".
      Default behavior is to print an error message.
        @param  st  State (index) we are interested in.
        @return true, iff from this state it is possible to reach
                      another state j, where from j we cannot reach st.
  */
  virtual bool isTransient(long st) const;


  /** Get the initial (time 0) distribution.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @return    Shallow copy of initial distribution, or 0 on error.
  */
  virtual statedist* getInitialDistribution() const;

      
  /** Get the outgoing weights from a given state.
      This really only makes sense for Markov chains
      and a few other special cases.
      The default behavior here is to print an error message.
        @param  from    Desired source state
        @param  to      Destination states are written here
        @param  w       Output weights are written here
        @param  n       Dimension of arrays \a to and \a w.  
                        
        @return Number of outgoing edges.  This can be larger than \a n.
                We will NOT write more than \a n items to the arrays.
  */
  virtual long getOutgoingWeights(long from, long* to, double* w, long n) const;

  /** Compute the distribution at time t.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  t       Time.
        @param  probs   An array of dimension getNumStates().
                        On input: the probability for each state at time 0.
                        On output: the probability for each state at time t.
        @param  aux     Auxiliary vector, dimension getNumStates().
        @param  aux2    Another auxiliary vector, dimension getNumStates(),
                        required only for CTMCs.

        @return    true on success, false otherwise.
  */
  virtual bool computeTransient(double t, double* probs, 
        double* aux, double* aux2) const;
 
  /** Accumulate expected time spent in each state, until time t.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  t       Time.
        @param  p0      An array of dimension getNumStates(), holding
                        the probability for each state at time 0.
        @param  n       An array of dimension getNumStates().
                        On output: the accumulated expected time 
                        spent in each state.
        @param  aux     Auxiliary vector, dimension getNumStates().
        @param  aux2    Another auxiliary vector, dimension getNumStates().

        @return    true on success, false otherwise.
  */
  virtual bool computeAccumulated(double t, const double* p0, double* n,
        double* aux, double* aux2) const;
 
  /** Compute the steady-state distribution.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  probs   An array of dimension getNumStates().
                        On input: ignored.
                        On output: the steady-state probability
                        for each state, where probs[i]
                        will be the probability for the
                        state returned by getStateNumber(i).

        @return    true on success, false otherwise.
  */
  virtual bool computeSteadyState(double* probs) const;

  /** Compute time spent per state.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  p0    An array of dimension getNumStates(), holding
                      the probability for each state at time 0.

        @param  x     An array of dimension getNumStates().
                      On output, x[s] will be the expected time spent in
                      state s, if s is a transient state.
                        
        @return    true on success, false otherwise.
  */
  virtual bool computeTimeInStates(const double* p0, double* x) const;

  /** Compute time spent per state, and probability of reaching
      each recurrent class.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  p0    An array of dimension getNumStates(), holding
                      the probability for each state at time 0.

        @param  x     An array of dimension getNumStates().
                      On output, x[s] will be the expected time spent in
                      state s, if s is a transient state; otherwise x[s]
                      will be the probability of eventually reaching the 
                      recurrent class containing state s.
                        
        @return    true on success, false otherwise.
  */
  virtual bool computeClassProbs(const double* p0, double* x) const;


  /**
      Compute the distribution of the time to reach the accepting state.
      Currently, used only for discrete phase types.
        @param  epsilon   Desired precision;
                          build the distribution up to N such that
                          the probability of reaching the accepting state
                          at time N or later, is less than epsilon.

        @param  dist      Output: a newly-allocated array of doubles
                          that hold the distribution

        @param  N         Output: size of array \a dist.

        @return    true on success, false otherwise.
  */
  virtual bool computeDiscreteTTA(double epsilon, double* &dist, int &N) const;


  /**
      Compute the distribution of the time to reach the accepting state.
      Currently, used only for continuous phase types.
        @param  dt        Time discretization.
                          We determine the PDF for times
                          0, dt, 2dt, 3dt, ...

        @param  epsilon   Desired precision;
                          build the distribution up to a time such that
                          the probability of remaining in some transient state
                          after that time, is less than epsilon.

        @param  dist      Output: a newly-allocated array of doubles
                          that hold the distribution

        @param  N         Output: size of array \a dist.

        @return    true on success, false otherwise.
  */
  virtual bool 
  computeContinuousTTA(double dt, double epsilon, double* &dist, int &N) const;


  /**
      Compute, for each possible starting state, the probability of
      eventually reaching the accepting state.

        @param  x   Vector of size (at least) number of states,
                    to hold the result.

        @return    true on success, false otherwise.
  */
  virtual bool reachesAccept(double* x) const;


  /**
      Compute, for each possible starting state, the probability of
      reaching the accepting state by time t.

        @param  t   Time we care about.
                    Anything less than 0 acts like 0.

        @oaram  x   Vector of size (at least) number of states,
                    to hold the result.

        @return    true on success, false otherwise.
  */
  virtual bool reachesAcceptBy(double t, double* x) const;

  

  /** Simulate a random walk through the process, until absorption.
      This is the discrete-time version.
        @param  st      Random stream to use
        @param  state   On input: the starting state.
                        On output: the final state.
        @param  final   Set of accepting states, or 0 for empty set.
        @param  maxt    Max stopping time, so we definitely terminate.
        @param  time    Time we first hit a final state; no larger than maxt.
        @return true    on success
                false   otherwise
  */
  virtual bool randomTTA(rng_stream &st, long &state, const stateset* final,
                          long maxt, long &elapsed);

  /** Simulate a random walk through the process, until absorption.
      This is the continuous-time version.
        @param  st      Random stream to use
        @param  state   On input: the starting state.
                        On output: the final state.
        @param  final   Set of accepting states, or 0 for empty set.
        @param  maxt    Max stopping time, so we definitely terminate.
        @param  time    Time we first hit a final state; no larger than maxt.
        @return true    on success
                false   otherwise
  */
  virtual bool randomTTA(rng_stream &st, long &state, const stateset* final,
                          double maxt, double &elapsed);

};

#endif  // INITIALIZERS_ONLY

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeStochasticLLM(exprman* em);

#endif  
