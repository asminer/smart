
#ifndef DSDE_HLM_H
#define DSDE_HLM_H

#include "../ExprLib/mod_def.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/mod_inst.h"
#include "../_IntSets/intset.h"

#include <vector>
// **************************************************************************
// *                                                                        *
// *                           model_event  class                           *
// *                                                                        *
// **************************************************************************

/** Abstract base class, for model events.
    The base class contains enough information for model analysis.

    Note: priorities may be specified as either integers,
    or as a list of dependencies (acyclic) but not both.
*/
class model_event : public model_var {
public:
  /// Possible firing types
  enum firing_type {
    /// Placeholder, we don't know yet
    Unknown,
    /// Nondeterministic.
    Nondeterm,
    /// Nondeterministic and hidden (like immediate).
    Hidden,
    /// Immediate (fires in zero time).
    Immediate,
    /// Timed, with exponential firing delay.
    Expo,
    /// Timed, with phase int firing delay.
    Phase_int,
    /// Timed, with phase real firing delay.
    Phase_real,
    /// Timed, with general distribution.
    Timed_general
  };
  static const char* nameOf(firing_type t);

private:
  /// Firing type of this event.
  firing_type FT;
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

  /// Priority level was set.
  bool has_prio_level;

  /// Priority level.
  int prio_level;

  /// List of events that have priority over us; dynamic.
  List <model_event>* prio_list_dynamic;

  /// List of events that have priority over us; static.
  model_event** prio_list;

  /// Number of events that have priority over us.
  int prio_length;

  enum {
    disabled = -1,
    unknown = 0,
    enabled = 1
  } enable_data;

  intset* enabling_level_dependencies;
  intset* enabling_variable_dependencies;

  intset* nextstate_level_dependencies;
  intset* nextstate_variable_dependencies;

  static void buildDepList(expr* e, intset* ld, intset* vd);
  
public:
  model_event(const symbol* wrapper, const model_instance* p);
  model_event(const char* fn, int line, const type* t, char* n, 
              const model_instance* p);
protected:
  void Init();
  virtual ~model_event();

public:
  inline firing_type getFiringType() const { return FT; }
  inline bool hasFiringType(firing_type f) const { return (f == FT); }
  inline bool actsLikeImmediate() const {
    return (Hidden == FT) || (Immediate == FT);
  }

  void display(OutputStream &s) const;

  /** Set the enabling expression for this event.
        @param e    An expression of type "proc bool" that evaluates to
                    true if the event is enabled in a given state.
                    This expression does not need to take event priorities
                    into account (those are "fixed" elsewhere).
  */
  void setEnabling(expr* e);

  /// Get the enabling expression.
  inline expr* getEnabling() const { return enabling; }

  /// Build the lists of dependencies for the enabling expression
  inline void buildEnablingDependencies(int num_levels, int num_vars) {
    if (0==enabling_level_dependencies) 
      enabling_level_dependencies = new intset(num_levels+1);
    if (0==enabling_variable_dependencies) 
      enabling_variable_dependencies = new intset(num_vars);
    buildDepList(enabling, 
      enabling_level_dependencies,
      enabling_variable_dependencies
    );
  }

  inline bool enablingDependsOnLevel(int k) const {
    DCASSERT(enabling_level_dependencies);
    return enabling_level_dependencies->contains(k);
  }

  inline bool enablingDependsOnVar(int sv) const {
    DCASSERT(enabling_variable_dependencies);
    return enabling_variable_dependencies->contains(sv);
  }

  /** Set the next state expression for this event.
      For now, it is assumed that the next state is unique.
      TO DO: figure out a reasonable extension for multiple next states.
        @param  e   An expression of type "proc state" that evaluates to 
                    the (unique) next state reached if the event fires.
                    The expression may assume that the event is enabled.
  */
  void setNextstate(expr* e);
  
  /// Get the next state expression.
  inline expr* getNextstate() const { return nextstate; }

  /// Build the lists of dependencies for the next state expression
  inline void buildNextstateDependencies(int num_levels, int num_vars) {
    if (0==nextstate_level_dependencies) 
      nextstate_level_dependencies = new intset(num_levels+1);
    if (0==nextstate_variable_dependencies) 
      nextstate_variable_dependencies = new intset(num_vars);
    buildDepList(nextstate, 
      nextstate_level_dependencies,
      nextstate_variable_dependencies
    );
  }

  inline bool nextstateDependsOnLevel(int k) const {
    DCASSERT(nextstate_level_dependencies);
    return nextstate_level_dependencies->contains(k);
  }

  inline bool nextstateDependsOnVar(int sv) const {
    DCASSERT(nextstate_variable_dependencies);
    return nextstate_variable_dependencies->contains(sv);
  }

  inline bool dependsOnLevel(int k) const {
    return enablingDependsOnLevel(k) || nextstateDependsOnLevel(k);
  }

  inline bool dependsOnVar(int k) const {
    return enablingDependsOnVar(k) || nextstateDependsOnVar(k);
  }

  /** Set the firing type for this event to be Non-deterministic.
      The current firing type must be "Unknown".
  */
  void setNondeterministic();

  /** Set the firing type for this event to be Hidden.
      The current firing type must be "Unknown".
  */
  void setHidden();

  /** Set the firing type for this event to be Timed of some sort.
      The exact firing type is determined from the type of the 
      distribution expression.
      The current firing type must be "Unknown".
        @param  d   The firing distribution to use for this event.
  */
  void setTimed(expr* d);

  /** Set the firing type for this event to be Immediate.
      The current firing type must be "Unknown".
  */
  void setImmediate();

  /// Get the firing distribution.
  inline expr* getDistribution() const { 
    DCASSERT( (Expo == FT) || (Phase_int == FT) || 
              (Phase_real == FT) || (Timed_general == FT) );
    return distro; 
  }

  /** Set the weight and weight class for this event.
      Unnecessary for Non-deterministic events or 
      timed events with continuous distributions.
        @param  wc      The weight class of the event.
        @param  weight  Post-selection priority weight expression.
                        This should have type "real" or "proc real".
  */
  void setWeight(int wc, expr *weight);

  /// Get the weight.
  inline expr* getWeight() const {
    DCASSERT((FT != Nondeterm) && (FT != Expo));
    return weight;
  }
  /// Get the weight class.
  inline int getWeightClass() const { return wc; }

  inline bool hasPriorityLevel()  const { return (has_prio_level); }
  inline int  getPriorityLevel()  const { return prio_level; }
  inline void setPriorityLevel(int pl) { 
    prio_level = pl; 
    has_prio_level = true;
  }

  inline void addToPriorityList(model_event* t) {
    if (0==prio_list_dynamic) {
      prio_list_dynamic = new List <model_event>;
    }
    prio_list_dynamic->Append(t);
    prio_length++;
  }

  /** Finish the event priority.
        @param  tmp       Auxiliary list of events.  Will be destroyed.
        @param  ignored   If nonzero, any events in the priority list
                          with a different priority level are removed
                          from the list, and added to the ignored list
                          (so that a warning may be displayed).
  */
  void finishPriorityInfo(List <model_event> &tmp, List <model_event> *ignored);

  void decideEnabled(traverse_data &x);
  inline void clearEnabled() { enable_data = unknown; }
  inline void setDisabled() { enable_data = disabled; }
  inline bool unknownIfEnabled() const { return unknown == enable_data; }
  inline bool knownEnabled() const { return enabled == enable_data; }
  inline bool isEnabled() const {
    DCASSERT(unknown != enable_data);
    return enabled == enable_data;
  }

};


// **************************************************************************
// *                                                                        *
// *                             dsde_hlm class                             *
// *                                                                        *
// **************************************************************************


/** Abstract base class for discrete-event, discrete-state models.
    Note that these are "high-level".
*/
class dsde_hlm : public hldsm {
private:
  static named_msg ignored_prio;
  friend class init_dsde;
  /// Low level model type.
  lldsm::model_type lltype;
  /// Number of "levels".
  // int num_levels;
protected:
  /// Array of state variables.
  model_statevar** state_data;
  /// Total number of state variables.
  int num_vars;
  /// Array of events.
  model_event** event_data;
  /// Total number of events.
  int num_events;
  /// Number of different priority levels 
  int num_priolevels;
  /// Last immediate event (plus one) by priority level
  int* last_immed;
  /// Last timed event (plus one) by priority level
  int* last_timed;

  /// Assertions, must be true in each model state.
  expr** assertions;
  /// Dimension of assertions array.
  int num_assertions;
public:
  dsde_hlm(const model_instance* p, model_statevar** sv, int nv, model_event** ed, int ne);
  virtual ~dsde_hlm();

  virtual lldsm::model_type GetProcessType() const;

  inline int getNumEvents() const { return num_events; }
  inline const model_event* readEvent(int i) const {
    CHECK_RANGE(0, i, num_events);
    DCASSERT(event_data);
    return event_data[i];
  }
  inline model_event* getEvent(int i) {
    CHECK_RANGE(0, i, num_events);
    DCASSERT(event_data);
    return event_data[i];
  }
  inline void showEvents(OutputStream &s) const {
    for (int i=0; i<num_events; i++) {
      DCASSERT(event_data[i]);
      event_data[i]->display(s);
    }
  }

  inline const model_statevar* readStateVar(int i) const {
    CHECK_RANGE(0, i, num_vars);
    DCASSERT(state_data);
    return state_data[i];
  }
  inline model_statevar* getStateVar(int i) {
    CHECK_RANGE(0, i, num_vars);
    DCASSERT(state_data);
    return state_data[i];
  }
  inline int getNumStateVars() const { return num_vars; }
  virtual int NumStateVars() const;
  virtual bool containsListVar() const;
  virtual void determineListVars(bool* ilv) const;
  // inline int fastNumLevels() const { return num_levels; }
  virtual void reindexStateVars(int &start);

  /**
      Set up state variables for a user-determined variable ordering.
  */
  void useDefaultVarOrder();

  /**
      Set up state variables for a SMART-determined variable ordering.
  */
  void useHeuristicVarOrder();

  /** Set the assertions, these must be true for every reachable state.
        @param  as    Array of assertion expressions (proc bool type)
        @param  na    Dimension of array \a as.
  */
  inline void setAssertions(expr** as, int na) {
    DCASSERT(0==assertions);
    assertions = as;
    num_assertions = na;
  }

  /** Check that the assertions hold.
        @param  x   x.current_state is the state we'll check.
                    x.answer will store a boolean result.
                      true: all assertions hold.
                      false: some assertion failed.
                      (We will print an error message).
  */
  void checkAssertions(traverse_data &x);

  // For now:

  /** Determine if a state is vanishing or not.
      That means there is at least one immediate or hidden event enabled.
        @param  x   x.current_state is the state we'll check.
                    x.answer will store a boolean result,
                    or other error condition as appropriate.
  */
  void checkVanishing(traverse_data &x);

  /** Generate a list of enabled events.
      Takes priority information into account.
      Assumes all events are untimed.

        @param  x       On input: x.current_state is current state.
                        On output: x.answer will be a normal boolean result,
                        if everything was fine; null or some other 
                        error condition otherwise.

        @param  enabled On output: enabled events are added to the list,
                        unless it is 0.  If an error occurs, the list will 
                        contain only the offending event(s):
                        length 1: event had bad enabling condition.
                        length 2: events enabled with different weight class
  */
  void makeEnabledList(traverse_data &x, List <model_event> *enabled);

  /// Like makeEnabledList, but we know the current state is vanishing.
  void makeVanishingEnabledList(traverse_data &x, List <model_event> *en);

  /// Like makeEnabledList, but we know the current state is tangible.
  void makeTangibleEnabledList(traverse_data &x, List <model_event> *en);


  // Things to be defined in derived classes.
  
  /** Returns the number of initial states for this model.
      For a Petri net, this should be 1.
  */
  virtual int NumInitialStates() const = 0;

  /** Fills the nth initial state for this model.
        @param  n  Initial state number, with
                      0 <= n < NumInitialStates().
        @param  s  Output: the state is stored here.
        @return The weight for this state
                (so we can build the initial distribution).
  */
  virtual double GetInitialState(int n, shared_state* s) const = 0;


  // Used for ordering events.
  inline int Compare(long i, long j) const {
    CHECK_RANGE(0, i, num_events);
    CHECK_RANGE(0, j, num_events);
    int jmi = event_data[j]->getPriorityLevel() 
            - event_data[i]->getPriorityLevel();
    if (jmi) return jmi;
    // same priority, now put immediate first.
    jmi = event_data[j]->actsLikeImmediate()
        - event_data[i]->actsLikeImmediate();
    if (jmi) return jmi;
    // priority and immediate-ness are equal; keep original order
    return event_data[i]->getID() - event_data[j]->getID();
  };
  // Swap events
  inline void Swap(long i, long j) {
    CHECK_RANGE(0, i, num_events);
    CHECK_RANGE(0, j, num_events);
    if (i!=j) SWAP(event_data[i], event_data[j]);
  }

protected:
  /// Reorder events and such, as convenient.
  void determineModelType();
  void ProcessEvents();
  inline void ResetEnabledList() {
    for (long e=num_events-1; e>=0; e--) {
      DCASSERT(event_data);
      DCASSERT(event_data[e]);
      event_data[e]->clearEnabled();
    }
  }
};


// **************************************************************************
// *                                                                        *
// *                             dsde_def class                             *
// *                                                                        *
// **************************************************************************


/** Abstract base class for high-level formalisms.
    Allows us to collect common things (right now, "partition"s)
    into a single spot.
*/
class dsde_def : public model_def {
  static named_msg dup_part;
  static named_msg no_part;
  static named_msg dup_prio;
  int last_level;
  friend class init_dsde;
public:
  dsde_def(const char* fn, int line, const type* t, char*n, 
      formal_param **pl, int np);

  virtual ~dsde_def();

  // for place partitions
  void SetLevelOfStateVars(const expr* call, int level, shared_set* pset);
  inline void SetLevelOfStateVars(const expr* call, shared_set* pset) {
    last_level--;
    SetLevelOfStateVars(call, last_level, pset);
  }
 
  void PartitionVars(model_statevar** V, int nv);

  // for event priorities
  void SetPriorityLevel(const expr* call, int level, shared_set* tset);
  void SetPriorityOver(const expr* call, shared_set* hight, shared_set* lowt);

protected:
  virtual void InitModel();
};

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

class symbol_table;
void Add_DSDE_varfuncs(const type* svt, symbol_table* syms);
void Add_DSDE_eventfuncs(const type* evt, symbol_table* syms);


#endif

