
// $Id$

#ifndef GRAPH_LLM_H
#define GRAPH_LLM_H

#ifndef INITIALIZERS_ONLY

#include "state_llm.h"

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
*/  
class graph_lldsm : public state_lldsm {
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

public:

  /**
      Reachability graphs.
      Abstract base class; different implementations provided
      by derived classes.
      Basically a Kripke structure, but without the labelling function.
      Could be a supergraph of the actual reachability graph,
      i.e., could contain states that are not reachable,
      but will never have edges from reachable states
      to unreachable ones.
  */
  class reachgraph : public shared_object {
      const graph_lldsm* parent;
    protected:
      static exprman* em;
    public:
      reachgraph();
    protected:
      virtual ~reachgraph();

    public:
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

      /** Get the number of edges in the reachability graph.
          The default version provided here will only work if
          the number of edges fits in a long.
            @param  na    Number of edges is stored here, as a "bigint" 
      */
      virtual void getNumArcs(result &na) const;  

      /** Get the number of edges in the reachability graph.
            @param  na    Number of edges is stored here
      */
      virtual void getNumArcs(long &na) const = 0;

      /** Show the internal representation of the reachability graph.
            @param  os    Output stream to write to
      */
      virtual void showInternal(OutputStream &os) const = 0;
    
      /**
        Show all the edges, in the desired order.
          @param  os    Output stream to write to
          @param  RSS   Reachable states
          @param  ord   Display order to use
          @param  st    Memory space to use for individual states
      */
      virtual void showArcs(OutputStream &os, reachset* RSS, 
        state_lldsm::display_order ord, shared_state* st) const = 0;


      /** Compute states satisfying EX(p).
          The default behavior here is to print an error message and 
          return null, so normally this method will be overridden in 
          some derived class.
            @param  revTime   If true, reverse time and compute EY.
            @param  p         Set of states 
            @return   New set of states satisfying EX(p) or EY(p).
                      OR, if an error occurs, prints an appropriate message
                      and returns 0.
      */
      virtual stateset* EX(bool revTime, const stateset* p) const;

      /** Compute states satisfying E p U q.
          The default behavior here is to print an error message and 
          return null, so normally this method will be overridden in 
          some derived class.
            @param  revTime   If true, reverse time and compute ES.
            @param  p         Set of states for p.  If 0, then
                              we instead compute EF / EP.
            @param  q         Set of states for q
            @return   New set of states satisfying E p U q
                      or E p S q.
                      OR, if an error occurs, prints an appropriate message
                      and returns 0.
      */
      virtual stateset* EU(bool revTime, const stateset* p, const stateset* q) const;

      /** Compute states satisfying EG(p), not restricted to fair paths.
          The default behavior here is to print an error message and 
          return null, so normally this method will be overridden in 
          some derived class.
            @param  revTime   If true, switch to unfairEH.
            @param  p         Set of states satisfying p.
            @return   New set of states satisfying EG(p).
                      OR, if an error occurs, prints an appropriate message
                      and returns 0.
      */
      virtual stateset* unfairEG(bool revTime, const stateset* p) const;

      /** Compute states satisfying EG(p), restricted to fair paths.
          The default behavior here is to print an error message and 
          return null, so normally this method will be overridden in 
          some derived class.
            @param  revTime   If true, switch to unfairEH.
            @param  p         Set of states satisfying p.
            @return   New set of states satisfying EG(p).
                      OR, if an error occurs, prints an appropriate message
                      and returns 0.
      */
      virtual stateset* fairEG(bool revTime, const stateset* p) const;

      /** Compute states satisfying AE p F q (made up notation).
          This is the set of source states, from which we can guarantee that
          we reach a state in q.  For states in p, we can choose the next state,
          otherwise we cannot choose the next state, and unfair paths are allowed.
          Thus AE false F q = AF q and AE true F q = EF q.

          Really useful for games or other control problems.
      
          The default behavior here is to print an error message and 
          return null, so normally this method will be overridden in 
          some derived class.

            @param  revTime   Why reverse time?  Because we can.
            @param  p         Set of controlled states
            @param  q         Set of goal states

            @return   New set of states satisfying AE p F q.
                      OR, if an error occurs, prints an appropriate message
                      and returns 0.
      */
      virtual stateset* unfairAEF(bool revTime, const stateset* p, const stateset* q) const;

      // Shared object requirements
      virtual bool Print(OutputStream &s, int width) const;
      virtual bool Equals(const shared_object* o) const;

    protected:
      static void showError(const char* str);
      static stateset* notImplemented(const char* op);
      friend void InitializeGraphLLM(exprman* em);
    };
    // ------------------------------------------------------------
    // end of inner class reachset


public:
  graph_lldsm(model_type t);

  virtual ~graph_lldsm();

  inline const reachgraph* getRGR() const {
    return RGR;
  }

  inline void setRGR(reachgraph* rgr) {
    DCASSERT(0==RGR);
    RGR = rgr;
  }

  inline void getNumArcs(result& count) const {
    DCASSERT(RGR);
    return RGR->getNumArcs(count);
  }

  virtual long getNumArcs() const {
    DCASSERT(RGR);
    long na;
    RGR->getNumArcs(na);
    return na;
  }

// TBD - rearrange from here

  /** Show the reachability graph or underlying process.
        @param  internal  If true, show internal details of state storage only.
                          If false, show a sane list of states, unless there
                          are too many to display.
  */
  void showArcs(bool internal) const;

  /// Check if na exceeds option, if so, show "too many arcs" message.
  static bool tooManyArcs(long na, bool show);

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

#else

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

  inline stateset* EX(bool revTime, const stateset* p) const {
    return RGR ? RGR->EX(revTime, p) : 0;
  }

  inline stateset* EU(bool revTime, const stateset* p, const stateset* q) const {
    return RGR ? RGR->EU(revTime, p, q) : 0;
  }

  inline stateset* unfairEG(bool revTime, const stateset* p) const {
    return RGR ? RGR->unfairEG(revTime, p) : 0;
  }

  inline stateset* fairEG(bool revTime, const stateset* p) const {
    return RGR ? RGR->fairEG(revTime, p) : 0;
  }

  inline stateset* unfairAEF(bool revTime, const stateset* p, const stateset* q) const {
    return RGR ? RGR->unfairAEF(revTime, p, q) : 0;
  }

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

private:
  reachgraph* RGR;

// Options
private:
  static long max_arc_display;
  static int graph_display_style;
  static bool display_graph_node_names;
protected:
  static named_msg numpaths_report;

  friend void InitializeGraphLLM(exprman* em);
};

#endif // INITIALIZERS_ONLY

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeGraphLLM(exprman* em);

#endif

