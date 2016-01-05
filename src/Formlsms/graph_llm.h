
// $Id$

#ifndef GRAPH_LLM_H
#define GRAPH_LLM_H

#include "../ExprLib/mod_inst.h"

#include "../Modules/statesets.h" // for now

class intset;
class stateset;

// ******************************************************************
// *                                                                *
// *                       graph_lldsm  class                       *
// *                                                                *
// ******************************************************************

/**   Class for graph models.
      Essentially, kripke structures, but without the labeling function.
      In other words, models that can be checked against properties
      expressed in temporal logics like LTL and CTL.

      TBD - redesigning this class:
        
        some virtual functions could be provided here
        some functions might be unnecessary
        some functions should be generalized 
          (with concurrent redesign of stateset class)


      TBD - reachset stuff should move to lldsm

      TBD - forward, backward should take an op name,
            for printing error messages 
*/  
class graph_lldsm : public lldsm {
public:
  graph_lldsm(model_type t);

  virtual ~graph_lldsm();

//
// Option-related stuff
//
public:
  enum display_style {
    DOT        = 0,
    INCOMING   = 1,
    OUTGOING   = 2,
    TRIPLES    = 3
  };
  static const int num_graph_display_styles = 4;

  inline static display_style graphDisplayStyle() {
    switch (graph_display_style) {
      case 0 : return DOT;
      case 1 : return INCOMING;
      case 2 : return OUTGOING;
      case 3 : return TRIPLES;
    }
    // Sane default
    return OUTGOING;
  }

  inline static bool displayGraphNodeNames() {
    return display_graph_node_names;
  }

  /// Check if na exceeds option, if so, show "too many arcs" message.
  static bool tooManyArcs(long na, bool show);


public:

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
  virtual long getNumArcs() const;

  /** Show the reachability graph or underlying process.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
        @param  internal  If true, show internal details of state storage only.
                          If false, show a sane list of states, unless there
                          are too many to display.
  */
  virtual void showArcs(bool internal) const;

  /** Show the initial state(s) of the graph.
      This must be provided in derived classes, the
      default behavior here is to print an error message.
  */
  virtual void showInitial() const;

#ifdef NEW_STATESETS
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
  virtual void countPaths(const stateset* src, const stateset* dest, result& count);

#else
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
#endif

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

#ifdef NEW_STATESETS

  /** Get the reachable states, as a stateset.
      Default behavior here is to print an error message and return null.
        @return   New stateset object for the reachable states,
                  or 0 on error.
  */
  virtual stateset* getReachable() const;

  /** Get the possible initial (time 0) states.
      Conceptually, this tells which elements in the vector 
      constructed by getInitialDistribution() will have 
      non-zero probability.  This must be provided in derived 
      classes, the default behavior here is to print an error 
      message and return null.
        @return   New stateset object for the initial states,
                  or 0 on error.
  */
  virtual stateset* getInitialStates() const;

  /** Get the (potential) states that, once entered, are never 
      left.  This includes deadlocked states.  This must be 
      provided in derived classes, the default behavior here 
      is to print an error message and return null.
        @return   New stateset object for the absorbing states,
                  or 0 on error.
  */
  virtual stateset* getAbsorbingStates() const;

  /** Get the (potential) deadlocked states.
      That means states that have no outgoing edges (even to itself).
      This must be provided in derived classes, the default behavior 
      here is to print an error message and return null.
        @return   New stateset object for the deadlocked states,
                  or 0 on error.
  */
  virtual stateset* getDeadlockedStates() const;

  /** Get the set of states satisfying a constraint.
      Default behavior here is to print an error message and return null.
        @param  p   Logical condition for states to satisfy.
                    If 0, we quickly return a new empty set.
        @return   New stateset object for states satisfying p,
                  or 0 on error.
  */
  virtual stateset* getPotential(expr* p) const;


#else

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

#endif

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

#ifdef NEW_STATESETS

  /** Determine TSCCs satisfying a property.
      This is done "in place".
      Should only be called for "fair" models.
      Default behavior is to print an error message.
         @param  p  On input: property p.
                    On output: states are removed if
                    they are not in a TSCC, or if
                    not all states in the TSCC satisfy p.
  */
  virtual void getTSCCsSatisfying(stateset* p) const;

  /** Find all "deadlocked" states.
      Default behavior is to print an error message.
        @param  ss  If i is not a deadlocked state,
                    then i will be removed from ss.
  */
  virtual void findDeadlockedStates(stateset* ss) const;

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
  virtual bool forward(const stateset* p, stateset* r) const;

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
  virtual bool backward(const stateset* p, stateset* r) const;

#else

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

#endif

  // Hacks for explicit:
  // TBD - move these?

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

public:

  /**
      Reachable states.
      Abstract base class; different implementations provided
      by derived classes.
  */
  class reachset : public shared_object {
      const graph_lldsm* parent;
    public:
      /**
          Abstract base class for different state orders.
      */
      class iterator {
        public:
          iterator();
          virtual ~iterator();
  
          /// Reset the iterator back to the beginning
          virtual void start() = 0;
  
          /// Increment the iterator
          virtual void operator++(int) = 0;
  
          /// Is the iterator still valid?
          virtual operator bool() const = 0;
  
          /// Return the index of the current state.
          virtual long index() const = 0;
  
          /// Copy the current state into st.
          virtual void copyState(shared_state* st) const = 0;
      };
  
    public:
      reachset();
      virtual ~reachset();
  
      inline void setParent(const graph_lldsm* p) {
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
  
      virtual void getNumStates(result &ns) const;  // default: use a long
      virtual void getNumStates(long &ns) const = 0;
      virtual void showInternal(OutputStream &os) const = 0;
      virtual void showState(OutputStream &os, const shared_state* st) const = 0;
      virtual iterator& iteratorForOrder(int display_order) = 0;
      virtual iterator& easiestIterator() const = 0;
  
      /*
          TBD - add a reachgraph parameter, needed for the stateset.
  
          TBD - adjust the stateset class and  use a proper class hierarchy.
  
      */
      virtual stateset* getReachable() const = 0;
      virtual stateset* getPotential(expr* p) const = 0;
      virtual stateset* getInitialStates() const = 0;
  
      /**
        Show all the states, in the desired order.
          @param  os             Output stream to write to
          @param  display_order  Display order to use.  See class lldsm
          @param  st             Memory space for use to use for individual states
      */
      void showStates(OutputStream &os, int display_order, shared_state* st);
  
      /**
        Visit all the states, in the desired order.
          @param  x             State visitor.
          @param  visit_order   Order to use, same as display_order constants in lldsm.
      */
      void visitStates(lldsm::state_visitor &x, int visit_order);

      /**
        Visit all the states, in any convenient order.
          @param  x             State visitor.
      */
      void visitStates(lldsm::state_visitor &x) const;

      // Shared object requirements
      virtual bool Print(OutputStream &s, int width) const;
      virtual bool Equals(const shared_object* o) const;
  };


public:

  /**
      Reachability graphs.
      Abstract base class; different implementations provided
      by derived classes.
      Basically a Kripke structure, but without the labelling function.
  */
  class reachgraph : public shared_object {
    const graph_lldsm* parent;
  public:
    reachgraph();
    virtual ~reachgraph();

    inline void setParent(const graph_lldsm* p) {
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

    // What virtual functions here?
    // virutal void getNumArcs(result &na) const;  // default: use a long
    // virtual void getNumArcs(long &na) const = 0;
    // virtual void showInternal(OutputStream &os);
    
    // which of these will belong here?

    // checkable requirements
    // virtual bool isAbsorbing(long st) const;
    // virtual bool isDeadlocked(long st) const;

    // virtual void findDeadlockedStates(stateset &ss) const;
    // virtual bool forward(const intset &p, intset &r) const;
    // virtual bool backward(const intset &p, intset &r) const;

    // virtual bool dumpDot(OutputStream &s) const;

    /**
      Show all the edges, in the desired order.
        @param  os    Output stream to write to
        @param  dispo Display order to use.  See class lldsm
        @param  st    Memory space to use for individual states

      TBD - reachset is needed, should it be a parameter or is it 
      needed for so much stuff that the class keeps a pointer to it?

    */
    void showArcs(OutputStream *os, int dispo, shared_state* st);

    // TBD?
    // void visitArcs();  to visit in any order

    // Shared object requirements
    virtual bool Print(OutputStream &s, int width) const;
    virtual bool Equals(const shared_object* o) const;

  };

private:
  reachset* RSS;
  reachgraph* RGR;

// Options
private:
  static long max_arc_display;
  static int graph_display_style;
  static bool display_graph_node_names;

  friend void InitializeCheckableLLM(exprman* em);
};

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeCheckableLLM(exprman* em);

#endif

