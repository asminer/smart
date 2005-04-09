
// $Id$

#ifndef DSM_H
#define DSM_H

#include "../Language/exprs.h"
#include "../States/stateheap.h"

/** @name dsm.h
    @type File
    @args \ 

  Discrete-state model.
  The meta model class used by solution engines.

  Hopefully this can avoid virtual functions for the critical operations.

*/

//@{

class reachset;  // defined in States/reachset.h
class reachgraph;  // defined in Chains/procs.h
class markov_chain;  // defined in Chains/procs.h


/** Meta-event class.
    
    Basically, we now force a specific formalism to "compile" down to
    these events, which completely describe state changes.
    The event class is derived from symbol, because it is useful for
    them to have names, types, and filename/linenumber where created.

    TODO STILL: figure out how to handle immediate events

    The mechanisms used are:

    enabling:	An expression of type "proc bool" that evaluates to
    		true if the event is enabled in a given state.
		This expression does not need to take event priorities
		into account (those are "fixed" outside of here).

    nextstate:	An expression either of type "proc state" or "proc stateset".
    		The expression may assume that the event is in fact enabled.

    		For proc state, we can determine the (unique) next state
		reached if the event fires.

		For proc stateset, we can determing the set of possible states
		reached if the event fires.

    distro:	The firing distribution, or NULL for "nondeterministic".
    		The expression may assume that the event is in fact enabled.
    		Type can be "int" or "real" for constants,
		"ph int", "ph real", "rand int", "rand real"
		for state independent random durations,
		and the "proc" versions of the above if durations
		are state dependent.
*/

class event : public symbol {
  /// Enabling expression
  expr* enabling;
  /// Nextstate expression
  expr* nextstate;
  /// Firing distribution
  expr* distro;
public:
  event(const char* fn, int line, type t, char* n);
  virtual ~event();

  void setEnabling(expr *e);
  void setNextstate(expr *e);
  void setDistribution(expr *e);

  inline expr* isEnabled() { return enabling; }
  inline expr* getNextstate() { return nextstate; }
  inline type  NextstateType() { return (nextstate) ? nextstate->Type(0) : VOID; }
  inline expr* Distribution() { return distro; }
  inline type  DistroType() { return (distro) ? distro->Type(0) : VOID; }

  // required to keep exprs happy
  virtual void ClearCache() { DCASSERT(0); }
};


/** Possible "stochastic processes" for a state model (below).
    The specific one is determined from the types of the firing
    distributions (if present) for the model.
*/
enum Process_type {
  /// Placeholder; we don't know yet
  Proc_Unknown,
  /// An unsupported process, or an error occurred
  Proc_Error,
  /// Finite state machine (no timing, for model checking)
  Proc_FSM,
  /// Discrete-time Markov chain
  Proc_Dtmc,
  /// Continuous-time Markov chain
  Proc_Ctmc,
  // other types, for Rob?

  /// General stochastic process (use simulation)
  Proc_General
};


/** Meta-model class.
    
    Also derived from "symbol" for the same reasons: to have the name,
    type, and filename/linenumber.

    Interface is significantly simplified thanks to event class (above).

    We will still use virtual functions and derived classes for:
	
	State information
		displaying, size, initial state, etc.

*/

class state_model : public symbol {
  int num_events;
  event** event_data;
public:  
  /// Type of the underlying process
  Process_type proctype;
  /// Reachable states.
  reachset *statespace; 

  // list out the different underlying processes here?
  // or use a union?
  // decide later...

  /// Reachability graph
  reachgraph *rg; 

  /// Markov chain (discrete or continuous)
  markov_chain *mc;

public:
  state_model(const char* fn, int line, type t, char* n, event** ed, int ne);
  virtual ~state_model();

  inline int NumEvents() const { return num_events; }
  inline event* GetEvent(int n) const {
    DCASSERT(event_data);
    CHECK_RANGE(0, n, num_events);
    return event_data[n];
  }

  void DetermineProcessType();

  /** Does this model use a state of constant size.
      True for Petri nets and Markov chains.
      False for colored nets (not implemented yet...)
  */
  virtual bool UsesConstantStateSize() const = 0;

  /** How large is a state for this model.
      Assumes the model uses a state of constant size.
      Behavior specified here is to bail out
      (which is what should be done if a state is NOT constant size).
  */
  virtual int  GetConstantStateSize() const;

  virtual void ShowState(OutputStream &s, const state &x) const = 0;

  /** Returns the number of initial states for this model.
      For a Petri net, this should be 1.
      For a Markov chain, this can be greater than 1.
  */
  virtual int NumInitialStates() const = 0;

  /** Fills the nth initial state for this model.
      0 <= n < NumInitialStates()
  */
  virtual void GetInitialState(int n, state &s) const = 0;

  // required to keep exprs happy
  virtual void ClearCache() { DCASSERT(0); }
};

// void Delete(state_model *x);

OutputStream& operator<< (OutputStream &s, state_model *m);

//@}

#endif

