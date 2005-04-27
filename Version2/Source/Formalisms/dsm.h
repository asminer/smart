
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

/** Possible type for meta-events (below).
*/
enum Event_type {
  /// Placeholder; we don't know yet
  E_Unknown,
  /// Nondeterministic.
  E_Nondeterm,
  /// Timed.
  E_Timed,
  /// Immediate.
  E_Immediate
};


/** Meta-event class.
    
    Basically, we now force a specific formalism to "compile" down to
    these events, which completely describe state changes.
    The event class is derived from symbol, because it is useful for
    them to have names, types, and filename/linenumber where created.

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

    distro:	Firing distribution for Timed events.
    		If the event type is Nondeterministic or Immediate, 
		this is NULL.
    		The expression may assume that the event is in fact enabled.
		
    		Type can be "int" or "real" for constants,
		"ph int", "ph real", "rand int", "rand real"
		for state independent random durations,
		and the "proc" versions of the above if durations
		are state dependent.

    weight:	Post-selection priority weight.
		The type should be either "real" for constant weights,
		or "proc real" for state-dependent weights.
		Will be NULL for sure for Nondeterministic events.
		Should be NULL for Timed events with continuous distributions.
		An error occurs if two or more events try to fire 
		simultaneously and do not have specified weights.
 
    wc:		Specifies the weight class.
	      	Should be 0 if no weight is defined.
		It is an error for events of different (nonzero) weight 
		classes to be enabled simultaneously.	
		
*/

class event : public symbol {
  friend class state_model;
  /// The type of event.
  Event_type ET;
  /// Enabling expression
  expr* enabling;
  /// Nextstate expression
  expr* nextstate;
  /// Firing distribution
  expr* distro;
  /// Weight
  expr* weight;
  /// Weight class
  int wc;
  /// List of events that we have priority over
  event** prio_list;
  /// Number of events we have priority over
  int prio_length;
  /// misc integer data, e.g., for enabling
  int misc;
public:
  /// Used by analysis engines; e.g., firing time for simulation
  double value;
public:
  event(const char* fn, int line, type t, char* n);
  virtual ~event();

  void setEnabling(expr *e);
  void setNextstate(expr *e);

  void setNondeterministic();
  void setTimed(expr *dist);
  void setImmediate();
  void setWeight(int wc, expr *weight);

  inline Event_type FireType() const { return ET; }

  inline expr* isEnabled() const { return enabling; }
  inline expr* getNextstate() const { return nextstate; }
  inline type  NextstateType() const { 
    return (nextstate) ? nextstate->Type(0) : VOID; 
  }
  inline expr* Distribution() const { 
    DCASSERT((E_Timed == ET) || (E_Immediate == ET));
    return distro; 
  }
  inline type  DistroType() const { 
    DCASSERT((E_Timed == ET) || (E_Immediate == ET));
    return (distro) ? distro->Type(0) : VOID; 
  }
  inline expr* Weight() const {
    DCASSERT((E_Timed == ET) || (E_Immediate == ET));
    return weight;
  }
  inline type WeightType() const {
    DCASSERT((E_Timed == ET) || (E_Immediate == ET));
    return (weight) ? weight->Type(0) : VOID;
  }
  inline int WeightClass() const { return wc; }

  // required to keep exprs happy
  virtual void ClearCache() { DCASSERT(0); }

  inline void SetPriorityList(event** pl, int pn) {
    DCASSERT(NULL==prio_list);
    prio_list = pl;
    prio_length = pn;
  }
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
protected:
  /// Total number of events
  int num_events;
  /// Number of immediate.
  int num_immediate;
  /// The events.
  event** event_data;
  /// Was there an error during construction?
  bool construct_error;
  /// Check for cycles and reorder events.
  void OrderEventsByPriority();
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

  inline bool BuildError() const { return construct_error; }

  inline int NumEvents() const { return num_events; }
  inline event* GetEvent(int n) const {
    DCASSERT(event_data);
    CHECK_RANGE(0, n, num_events);
    return event_data[n];
  }

  /** List of enabled events.
      Takes priority information into account.
      Returns false if there was an error, true on success.
  */
  bool GetEnabledList(const state &current, List <event> *enabled);

  /** Returns true if the state is vanishing.
      I.e., true iff at least one immediate event is enabled.

      @param	current		The state to check
      @param	x		Where to store the result.
				(Used to catch any errors.)
  */
  inline void isVanishing(const state &current, result &x) {
    // check in order; no need to correct for priority!
    x.bvalue = false;
    for (int e=0; e<num_immediate; e++) {
      event_data[e]->enabling->Compute(current, 0, x);
      if (!x.isNormal()) return;
      if (x.bvalue) return;  // an immediate is enabled.
    }
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

  /** For enumerated state models.
      Obtain the ith state.
  */
  virtual void GetState(int n, state &x) const;

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

