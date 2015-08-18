
// $Id$

#ifndef MOD_INST_H
#define MOD_INST_H

/** \file mod_inst.h

    Instantiated models.

    Also, mechanisms for obtaining measures from instantiated models.

 */

#include "symbols.h"

class measure;
class set_of_measures;
class model_def;

class shared_state;
class model_statevar;
class hldsm;
class intset;

class subengine;
  
// ******************************************************************
// *                                                                *
// *                          lldsm  class                          *
// *                                                                *
// ******************************************************************

/** The base class of low-level discrete state models.
    New in this version: low-level models can be "partially generated"
    (for example, states only) and then we can continue construction
    using a specific (sub)engine.
*/  
class lldsm : public shared_object {
  /// Engine to call for continuing to build the model.
  subengine* next_phase;
  // options, shared by multiple low-level models.
  static const char* max_state_display_option;
  static long max_state_display;
protected:
  static int display_order;
  static const int DISCOVERY   = 0;
  static const int LEXICAL   = 1;
  static const int NATURAL   = 2;
  static const exprman* em;
  static named_msg numpaths_report;
  friend void InitLLM(exprman *em);

public:
  // class for visiting states.
  class state_visitor {
  protected:
    traverse_data x;
  public:
    state_visitor(const hldsm* m);
    virtual ~state_visitor();
    inline long&  index() { return x.current_state_index; }
    inline shared_state* state() { return x.current_state; }

    /** Return true iff we can skip state with index x.current_state_index.
    */
    virtual bool canSkipIndex() { return false; }

    /** Visit state; return true iff we can stop now.
        State to be visited is x.current_state, and it has
        index x.current_state_index.
    */
    virtual bool visit() { return false; }
  };
public:
  /// Possible types of low level models.
  enum model_type {
    /// Placeholder, we don't know the type yet.
    Unknown,
    /// Error; if we couldn't build the low level model
    Error,
    /// List of reachable states only
    RSS,
    /// Finite state machine (i.e., graph)
    FSM,
    /// Discrete-time Markov chain
    DTMC,
    /// Continuous-time Markov chain
    CTMC,

    /// General stochastic process
    GSP

    // TBD: other types here?
  };
private:
  model_type type;
protected:
  const hldsm* parent;
public:
  lldsm(model_type t);
protected:
  virtual ~lldsm();
  virtual const char* getClassName() const = 0;
public:
  inline model_type Type() const { return type; }
  inline void SetParent(const hldsm* p) {
    DCASSERT(0==parent || p==parent); 
    parent = p;
  }
  inline const hldsm* GetParent() const { return parent; }

  /// Set the engine to use to complete generation.
  inline void setCompletionEngine(subengine* e) { next_phase = e; }

  /// Get the engine to use to complete generation.
  inline subengine* getCompletionEngine() { return next_phase; }

  static const char* getNameOf(model_type t);

  // shared object requirements:
  virtual bool Print(OutputStream &, int) const;
  virtual bool Equals(const shared_object*) const;
  
  /** Get the number of reachable states.
      This version is used to implement Smart function num_states.
      The default version provided here will only work if
      the number of states fits in a long.
        @param  count   Number of states is stored here,
                        as a "bigint" if that type exists and there are
                        a large number of states, otherwise as a long.
  */
  virtual void getNumStates(result& count) const;

  /** Get the number of reachable states.
      This must be provided in derived classes, the
      default behavior here is to print an error message.

        @return  The number of reachable states, if it fits in a long;
                -1, otherwise (on overflow).
  */
  virtual long getNumStates() const;

  /** Show the reachable states.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  internal  If true, show internal details of state storage only.
                          If false, show a sane list of states, unless there
                          are too many to display.
  */
  virtual void showStates(bool internal) const;

  /// Check if ns exceeds option, if so, show "too many states" message.
  static bool tooManyStates(long ns, bool show);

  /** Visit all our states, explicitly, in a convenient order.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  x   Specifies what we do when visiting each state.
                    In practice, will be a derived class.
  */
  virtual void visitStates(state_visitor &x) const;

  /** Get the specified enumerated state.
      Default behavior is to return 0.
        @param  i   Index of enumerated state to return.
        @return     0, if i is out of range or this is not
                       an enumerated state model.
                    A (copy) pointer to the actual state, otherwise.
                    The pointer should be Delete()'d when done.
  */
  virtual shared_object* getEnumeratedState(long i) const;

  /** Write memory information to the reporting stream.
        @param  em      Will write to the report stream of this manager.
        @param  prefix  Prefix to write before each line.
  */
  virtual void reportMemUsage(exprman* em, const char* prefix) const;

  // other virtual functions here
protected:
  /** Start an appopriate internal error message.
         @return  -2, for convenience.
  */
  long bailOut(const char* fn, int ln, const char* why) const;
};

// ******************************************************************
// *                                                                *
// *                       graph_lldsm  class                       *
// *                                                                *
// ******************************************************************

/** The base class of low-level discrete state models with graphs.
*/  
class graph_lldsm : public lldsm {
  // options
  static const char* max_arc_display_option;
  static long max_arc_display;
protected:
  static int graph_display_style;
  static const int DOT        = 0;
  static const int INCOMING   = 1;
  static const int OUTGOING   = 2;
  static const int TRIPLES    = 3;
  static bool display_graph_node_names;
  friend void InitLLM(exprman *em);
public:
  graph_lldsm(model_type t);

  /** Get the number of edges in the reachability graph.
      The default version provided here will only work if
      the number of edges fits in a long.
        @param  count   Number of edges is stored here,
                        as a "bigint" if that type exists and there are
                        a large number of edges, otherwise as a long.
  */
  virtual void getNumArcs(result& count) const;

  /** Get the number of edges in the reachability graph.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  show  If true, the graph / MC is displayed 
                      to the Output stream, unless the function
                      returns -1.

        @return  The number of edges, if it fits in a long;
                -1 otherwise (on overflow).
  */
  virtual long getNumArcs(bool show) const;

  /** Show the initial state(s) of the graph.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
  */
  virtual void showInitial() const;

  /** Count number of paths from src to dest in reachability graph.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  src     Set of starting states.
        @param  dest    Set of destination states.
        @param  count   On return, the number of distinct paths
                        from some starting state, that ends in
                        a destination state.  Will be infinite
                        if there is a loop on any path from
                        a starting state to a destination state.
  */
  virtual void countPaths(const intset &src, const intset &dest, result& count);

  /** Change our internal structure so as to be efficient "by rows".
      If this is already the case, do nothing.
        @param  rep   0, or pointer to reporting structure.
        @return true, if the operation was successful.
  */
  virtual bool requireByRows(const named_msg* rep);

  /** Change our internal structure so as to be efficient "by columns".
      If this is already the case, do nothing.
        @param  rep   0, or pointer to reporting structure.
        @return true, if the operation was successful.
  */
  virtual bool requireByCols(const named_msg* rep);

  /** Obtain the outgoing edges from a state.
      Requires that the graph is efficient "by rows".
        @param  from    Source state
        @param  e       If e is a valid list (not 0), then
                        destination states are added to e.
        @return Number of edges, on success; -1 on failure.
  */
  virtual long getOutgoingEdges(long from, ObjectList <int> *e) const;

  /** Obtain the incoming edges to a state.
      Requires that the graph is efficient "by columns".
        @param  to      Destination state
        @param  e       If e is a valid list (not 0), then
                        source states are added to e.
        @return Number of edges, on success; -1 on failure.
  */
  virtual long getIncomingEdges(long from, ObjectList <int> *e) const;

  /** Obtain the number of outgoing edges for each state.
        @param  a   For each state s, add the number of
                    outgoing edges for state s to a[s].
        @return true, if successful.
  */
  virtual bool getOutgoingCounts(long* a) const;

  /** Produce a "dot" file of this model.
      Default behavior is to (quietly) return false.
        @param  s    Stream to write to
        @return true on success, false otherwise.
  */
  virtual bool dumpDot(OutputStream &s) const;
protected:
  /// Check if na exceeds option, if so, show "too many arcs" message.
  static bool tooManyArcs(long na, bool show);
};

// ******************************************************************
// *                                                                *
// *                          hldsm  class                          *
// *                                                                *
// ******************************************************************

/** The base class of high-level discrete-state models.
*/
class hldsm : public shared_object {
protected:
  static const exprman* em;
  friend void InitLLM(exprman *em);
public:
  /** Possible types of high level models.
      Use negatives for anything that will NEVER require solution engines.
      Zero is reserved.
  */
  enum model_type {
    /// Error
    Error = -2,
    /// Placeholder, we don't know the type yet.
    Unknown = -1,
    /// Reserved for non-models (functions).
    Nothing = 0,
    /// We are actually an "enumerated" low-level model.
    Enumerated = 1,
    /// Phase-type model
    Phase_Type = 2,
    /// Models with no events (used for optimization problems)
    No_Events = 3,
    /// Events are asynchronous 
    Asynch_Events = 4,
    /// Events are synchronous (e.g., all phase ints)
    Synch_Events = 5,
    /// General case for events
    General_Events = 6

    // TBD: other types here?
  };

  // For engine registration and such
  static const int Last_Model_Type = 6;

  /** Partition information.
      Used to group state variables "by level".
      We use a data structure similar to "row pointer column index" sparse
      matrix storage, except pointers are "in reverse".
  */
  struct partinfo {
    /// Number of levels of state variables.
    int num_levels;
    /** Pointer to first state variable, per level.  
        An array of dimension \a 1+num_levels.
        pointer[k] gives the index of the last element in array \a variable
        that is assigned to level k.
        pointer[0] gives -1.
    */
    int* pointer;
    /// Total number of state variables.
    int num_vars;
    /** State variables, ordered by level.
        An array of dimension \a num_vars.
    */
    const model_statevar** variable;
    
    friend class hldsm; 
    protected:
    /// Constructor.  Sets up everything.
    partinfo(int NL, int NV, model_statevar** vars);
    /// Destructor.
    ~partinfo();
  };


private:
  model_type type;
  const symbol* parent; 
  /// Saved partition information.
  partinfo* part;
protected:
  lldsm* process;
public:
  hldsm(model_type t);
protected:
  virtual ~hldsm();
public:
  inline model_type Type() const { return type; } 
  inline void setType(model_type t) {
    DCASSERT(Unknown == type);
    type = t;
  }

  /// Number of state variables.
  virtual int NumStateVars() const = 0;

  /// Does the model contain any "list" variables?
  virtual bool containsListVar() const = 0;

  /** Determine which state variables are list variables.
        @ilv  Array of dimension NumStateVars().
              On output, ilv[sv] will be true iff
                state variable sv is a list variable.
  */
  virtual void determineListVars(bool* ilv) const = 0;

  inline void SetParent(const symbol* p) {
    DCASSERT(0==parent || p==parent);
    parent = p;
  }
  inline const symbol* GetParent() const {
    return parent;
  }
  inline const char* Name() const {
    return parent ? parent->Name() : 0;
  }
  inline void SetProcess(lldsm* proc)  { 
    process = proc; 
    if (proc) proc->SetParent(this);
  }
  inline lldsm* GetProcess()     { return process; }
  
  virtual lldsm::model_type GetProcessType() const = 0;

  inline bool hasPartInfo() const { return part; }
  inline const partinfo& getPartInfo() const {
    DCASSERT(part);
    return *part; 
  }
protected:
  /// Note: \a vars is NOT modified.
  inline void setPartInfo(int NL, int NV, model_statevar** vars) {
    DCASSERT(0==part);
    if (NL>1) part = new partinfo(NL, NV, vars);
  }
public:

  // shared object requirements:
  virtual bool Print(OutputStream &, int) const;
  virtual bool Equals(const shared_object*) const;

  /** Start a warning message.
      Returns true on success.
  */
  bool StartWarning(const named_msg &who, const expr* cause) const;

  /** Finish a warning message.
  */
  void DoneWarning() const;
  
  /** Start an error message.
      Returns true on success.
  */
  bool StartError(const expr* cause) const;

  /// Print an out of bounds error message, if appropriate.
  void OutOfBoundsError(const result &x) const;

  /// Send a simple error string
  void SendError(const char* s) const;

  /// Send an error value
  void SendRealError(const result &x) const;
 
  /** Finish an error message.
  */
  void DoneError() const;

  /// For debugging messages
  static const exprman* getEM() { return em; }

  /** Re-index the state variables.
      This is necessary for model hierarchies.
        @param  start   Input: first state index to use
                        Output: 1+last state index used
  */
  virtual void reindexStateVars(int &start) = 0;

  /** Display a state.
      Must be provided in derived classes
      (because only a formalism knows how to interpret its state).

      @param  s  Stream to write to
      @param  x  The state
  */
  virtual void showState(OutputStream &s, const shared_state* x) const = 0;

protected:
  /// Start an appopriate internal error message.
  void bailOut(const char* fn, int ln, const char* why) const;
};

// ******************************************************************
// *                                                                *
// *                      model_instance class                      *
// *                                                                *
// ******************************************************************

/**   A generic, fully-instantiated model.

      Essentially, this is the proper result type to return for a "model".
      I.e., the result generated by a statement such as...

      model m := dining_philosophers(100);

*/  
class model_instance : public symbol {
public:
  /// Possible states of a model instance
  enum instance_state {
    /// Instance is still being constructed
    Constructing,
    /// An error occurred during construction
    Error,
    /// The instance is built
    Ready,
    /// The instance has been deleted
    Deleted
  };

private:
  instance_state state;
  // bool is_submodel;

  /// Total number of accepted measures
  int num_accepted_msrs;

  /// List of groups of measures, sorted by engine type.
  set_of_measures** mgroups;
  /// Number of measure groups.
  int num_groups;

  /// Discrete-state, compiled "meta" model used by engines.
  hldsm* compiled;

  /** Symbols that can be exported (usually measures or arrays of measures).
      Stored as an array of symbols, sorted by name.
  */
  symbol **stab;  
  /// Number of entries in array stab.
  int num_symbols;  

  /** List of owned symbols.
      Held only so they can be trashed when the model instance is trashed.
  */
  symbol* slist;

public:
  model_instance(const char* fn, int line, const model_def* defn);
protected:
  virtual ~model_instance();
public:
  inline instance_state GetState() const { return state; }
  inline hldsm* GetCompiledModel() const { return compiled; }

  /** Start a warning message.
      Returns true on success.
  */
  bool StartWarning(const named_msg &who, const expr* cause) const;

  /** Finish a warning message.
  */
  void DoneWarning() const;
  
  /** Start an error message.
      Returns true on success.
  */
  bool StartError(const expr* cause) const;
  
  /** Finish an error message.
  */
  void DoneError() const;

  /** Add a symbol to our list of symbols to destroy when
      the model instance is destroyed.  Currently this 
      includes arrays of model variables and functions.
      External symbols (e.g., measures) should instead
      be given to the external symbol list.
  */
  void AcceptSymbolOwnership(symbol *a);

  /** Instantiate a symbol visible to the user.
        @param  slot  Slot number to store the symbol.
        @param  s     Symbol to store.
  */
  void AcceptExternalSymbol(int slot, symbol* s);

  /** Obtain a symbol visible to the user.
        @param  slot  Slot number of the symbol to retrieve.
        @return The symbol in the specified slot.
  */
  inline symbol* FindExternalSymbol(int slot) {
    DCASSERT(stab);
    CHECK_RANGE(0, slot, num_symbols);
    return stab[slot];
  }

  /** Instantiate a measure.
      This must be called whenever a measure is instantiated
      (handled by an appropriate statement).
  */
  void AcceptMeasure(measure *m);

  /** Group a measure.
      This means that we now know the engine type for the measure,
      and it can be added to the group for that type, as appropriate.
  */
  void GroupMeasure(measure* m);

  /** Solve a given measure.
      If the measure is part of a group, all measures in the group
      are solved.
  */
  void SolveMeasure(traverse_data &x, measure *m);

  /** Signifies that construction is complete.
        @param  model  A high-level, "compiled" discrete-state model.
  */
  void SetConstructionSuccess(hldsm* model);

  /// Signifies that there was a fatal error during construction
  void SetConstructionError();

  /// Signifies that the instance will no longer be used
  void Deconstruct();

  /** Check if the model has been instantiated properly.
      Should be called before computing measures and such.
        @param  call  Expression that calls us, used to grab the
                      filename and line number for any error messages.
        @param  who   Who requires us to be ready.
        @return false If the model state is \a IS_Ready.
                true  otherwise, and an error message is
                      displayed as appropriate.
  */
  bool NotProperInstance(const expr* call, const char* who) const;

};


// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

/// Make an "error" low-level model.
lldsm* MakeErrorModel();

/// Make a high-level wrapper around a low-level (enumerated) model.
hldsm* MakeEnumeratedModel(lldsm* mdl);


/// Initialize low-level model options.
void InitLLM(exprman* om);

#endif

