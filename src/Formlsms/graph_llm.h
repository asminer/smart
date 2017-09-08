
#ifndef GRAPH_LLM_H
#define GRAPH_LLM_H

#include "state_llm.h"

#include "../Modules/statesets.h" // for now
#include "../Modules/trace.h"

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

public:
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
    public:
      struct show_options {
        state_lldsm::display_order ORDER;
        graph_lldsm::display_style STYLE;
        bool NODE_NAMES;
        bool RG_ONLY;   // ?
      };
    private:
      const graph_lldsm* parent;
    protected:
      static named_msg ctl_report;
      static named_msg numpaths_report;
      static exprman* em;
    public:
      reachgraph();
    protected:
      virtual ~reachgraph();
      virtual const char* getClassName() const = 0;

      /**
          Hook for any desired preprocessing.
          The reachable states are also given, in case finishing the 
          reachability graph requires renumbering the states.
          Default behavior simply sets the parent.
      */
      virtual void attachToParent(graph_lldsm* p, state_lldsm::reachset* rss);

    public:
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
          @param  opt   Display options
          @param  RSS   Reachable states
          @param  st    Memory space to use for individual states
      */
      virtual void showArcs(OutputStream &os, const show_options &opt, 
        reachset* RSS, shared_state* st) const = 0;


      /** Compute states satisfying EX(p).
          The default behavior here is to print an error message and 
          return null, so normally this method will be overridden in 
          some derived class.
            @param  revTime   If true, reverse time and compute EY.
            @param  p         Set of states.
            @param  td        On return, necessary data (if provided) which can be
                              used for witness generation later.
            @return   New set of states satisfying EX(p) or EY(p).
                      OR, if an error occurs, prints an appropriate message
                      and returns 0.
      */
      virtual stateset* EX(bool revTime, const stateset* p, trace_data* td = nullptr);

      /** Compute states satisfying AX(p).
          The default behavior here is to print an error message and 
          return null, so normally this method will be overridden in 
          some derived class.
            @param  revTime   If true, reverse time and compute AY.
            @param  p         Set of states 
            @return   New set of states satisfying AX(p) or AY(p).
                      OR, if an error occurs, prints an appropriate message
                      and returns 0.
      */
      virtual stateset* AX(bool revTime, const stateset* p);

      /** Compute states satisfying E p U q.
          The default behavior here is to print an error message and 
          return null, so normally this method will be overridden in 
          some derived class.
            @param  revTime   If true, reverse time and compute ES.
            @param  p         Set of states for p.  If 0, then
                              we instead compute EF / EP.
            @param  q         Set of states for q.
            @param  td        On return, necessary data (if provided) which can be
                              used for witness generation later.
            @return   New set of states satisfying E p U q
                      or E p S q.
                      OR, if an error occurs, prints an appropriate message
                      and returns 0.
      */
      virtual stateset* EU(bool revTime, const stateset* p, const stateset* q, trace_data* td = nullptr);

      /** Compute states satisfying A p U q, not restricted to fair paths.
          The default behavior here is to print an error message and 
          return null, so normally this method will be overridden in 
          some derived class.
            @param  revTime   If true, reverse time and compute AS.
            @param  p         Set of states for p.  If 0, then
                              we instead compute AF / AP.
            @param  q         Set of states for q
            @return   New set of states satisfying A p U q
                      or A p S q.
                      OR, if an error occurs, prints an appropriate message
                      and returns 0.
      */
      virtual stateset* unfairAU(bool revTime, const stateset* p, const stateset* q);


      /** Compute states satisfying A p U q, restricted to fair paths.
          The default behavior here is to print an error message and 
          return null, so normally this method will be overridden in 
          some derived class.
            @param  revTime   If true, reverse time and compute AS.
            @param  p         Set of states for p.  If 0, then
                              we instead compute AF / AP.
            @param  q         Set of states for q
            @return   New set of states satisfying A p U q
                      or A p S q.
                      OR, if an error occurs, prints an appropriate message
                      and returns 0.
      */
      virtual stateset* fairAU(bool revTime, const stateset* p, const stateset* q);


      /** Compute states satisfying EG(p), not restricted to fair paths.
          The default behavior here is to print an error message and 
          return null, so normally this method will be overridden in 
          some derived class.
            @param  revTime   If true, switch to unfairEH.
            @param  p         Set of states satisfying p.
            @param  td        On return, necessary data (if provided) which can be
                              used for witness generation later.
            @return   New set of states satisfying EG(p).
                      OR, if an error occurs, prints an appropriate message
                      and returns 0.
      */
      virtual stateset* unfairEG(bool revTime, const stateset* p, trace_data* td = nullptr);

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
      virtual stateset* fairEG(bool revTime, const stateset* p);


      /** Compute states satisfying AG(p).
          The default behavior here is to print an error message and 
          return null, so normally this method will be overridden in 
          some derived class.
            @param  revTime   If true, reverse time and compute AH.
            @param  p         Set of states 
            @return   New set of states satisfying AG(p) or AH(p).
                      OR, if an error occurs, prints an appropriate message
                      and returns 0.
      */
      virtual stateset* AG(bool revTime, const stateset* p);


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
      virtual stateset* unfairAEF(bool revTime, const stateset* p, const stateset* q);

      /** Count number of paths from src to dest in reachability graph.
          This must be provided in derived classes, the
          default behavior here is to print an error message.
            @param  src     Set of starting states.
            @param  dest    Set of destination states.
            @param  count   On return, the number (as a bigint) of distinct 
                            paths from some starting state, that ends in a 
                            destination state.  Will be infinite if there is 
                            a loop on any path from a starting state to a 
                            destination state.
      */
      virtual void countPaths(const stateset* src, const stateset* dest, 
          result& count);

      /** Compute a trace verifying EX(q).
          The default behavior here is to print an error message,
          so normally this method will be overridden in
          some derived class.
            @param  revTime   If true, reverse time and compute EY.
            @param  p         Set of initial states (should include only one state).
            @param  td        Necessary data for witness generation.
            @param  ans       On return, a trace as a sequence of states.
      */
      virtual void traceEX(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans);

      /** Compute a trace verifying EU.
          The default behavior here is to print an error message,
          so normally this method will be overridden in
          some derived class.
            @param  revTime   If true, reverse time and compute ES.
            @param  p         Set of initial states (should include only one state).
            @param  td        Necessary data for witness generation.
            @param  ans       On return, a trace as a sequence of states.
      */
      virtual void traceEU(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans);

      /** Compute a trace verifying EG.
          The default behavior here is to print an error message,
          so normally this method will be overridden in
          some derived class.
            @param  revTime   If true, reverse time and compute EH.
            @param  p         Set of initial states (should include only one state).
            @param  td        Necessary data for witness generation.
            @param  ans       On return, a trace as a sequence of states.
      */
      virtual void traceEG(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans);

      virtual trace_data* makeTraceData() const;

      // Shared object requirements
      virtual bool Print(OutputStream &s, int width) const;
      virtual bool Equals(const shared_object* o) const;

    protected:
      static bool reportCTL();
      static void reportIters(const char* who, long iters);
      static void showError(const char* str);
      stateset* notImplemented(const char* op) const;
      stateset* incompatibleOperand(const char* op) const;
      friend class init_graphllm;
      friend class graph_lldsm; // overkill
    };
    // ------------------------------------------------------------
    // end of inner class reachset


public:
  graph_lldsm(model_type t);
protected:
  virtual ~graph_lldsm();
  virtual const char* getClassName() const;

public:
  inline const reachgraph* getRGR() const {
    return RGR;
  }

  inline reachgraph* useRGR() {
    return RGR;
  }

  inline void setRGR(reachgraph* rgr) {
    DCASSERT(0==RGR);
    RGR = rgr;
    if (RGR) RGR->attachToParent(this, RSS);
  }

  inline void getNumArcs(result& count) const {
    DCASSERT(RGR);
    return RGR->getNumArcs(count);
  }

  inline long getNumArcs() const {
//  virtual long getNumArcs() const {
    DCASSERT(RGR);
    long na;
    RGR->getNumArcs(na);
    return na;
  }

// TBD - rearrange from here

  /** Show the reachability graph.
        @param  internal  If true, show internal details of state storage only.
                          If false, show a sane graph, unless there
                          are too many edges to display.
  */
  void showArcs(bool internal) const;

  /** Compare number of edges with option.
        @param  ns    Number of states
        @param  os    If not null, display "too many arcs" message as appropriate.
        @return true  iff the number of edges exceeds the option.
  */
  static bool tooManyArcs(long na, OutputStream *os);

  inline bool tooManyArcs(OutputStream *os) const {
    return tooManyArcs(getNumArcs(), os);
  }

  /** Produce a "dot" file of this model.
      Equivalent to showArcs with a style of DOT.
  */
  void dumpDot(OutputStream &s) const;


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


  inline stateset* EX(bool revTime, const stateset* p, trace_data* td = nullptr) const {
    return RGR ? RGR->EX(revTime, p, td) : 0;
  }

  inline stateset* EU(bool revTime, const stateset* p, const stateset* q, trace_data* td = nullptr) const {
    return RGR ? RGR->EU(revTime, p, q, td) : 0;
  }

  inline stateset* unfairEG(bool revTime, const stateset* p, trace_data* td = nullptr) const {
    return RGR ? RGR->unfairEG(revTime, p, td) : 0;
  }

  inline stateset* fairEG(bool revTime, const stateset* p) const {
    return RGR ? RGR->fairEG(revTime, p) : 0;
  }

  inline stateset* AX(bool revTime, const stateset* p) const {
    return RGR ? RGR->AX(revTime, p) : 0;
  }

  inline stateset* unfairAU(bool revTime, const stateset* p, const stateset* q) const {
    return RGR ? RGR->unfairAU(revTime, p, q) : 0;
  }

  inline stateset* fairAU(bool revTime, const stateset* p, const stateset* q) const {
    return RGR ? RGR->fairAU(revTime, p, q) : 0;
  }

  inline stateset* AG(bool revTime, const stateset* p) const {
    return RGR ? RGR->AG(revTime, p) : 0;
  }


  inline stateset* unfairAEF(bool revTime, const stateset* p, const stateset* q) const {
    return RGR ? RGR->unfairAEF(revTime, p, q) : 0;
  }

  inline void countPaths(const stateset* src, const stateset* dest, result& count) const
  {
    if (RGR) {
      RGR->countPaths(src, dest, count);
    } else {
      count.setNull();
    }
  }

  inline void traceEX(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans) const {
    if (RGR) {
      RGR->traceEX(revTime, p, td, ans);
    }
  }

  inline void traceEU(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans) const {
    if (RGR) {
      RGR->traceEU(revTime, p, td, ans);
    }
  }

  inline void traceEG(bool revTime, const stateset* p, const trace_data* td, List<stateset>* ans) const {
    if (RGR) {
      RGR->traceEG(revTime, p, td, ans);
    }
  }

  inline trace_data* makeTraceData() const {
    return RGR ? RGR->makeTraceData() : 0;
  }

  // Hacks for explicit:
  //
  // isAbsorbing is used (only?) by mc_form and fsm_form, so move it to reachgraph class
  //    or better yet to the appropriate derived class
  //

  /** Is the given state "absorbing".
      This means that either there are no outgoing edges from this state,
      OR there is a single outgoing edge, to itself.
      Default behavior is to print an error message.
        @param  st    State (index) we are interested in.
        @return true, iff it is impossible to leave state st.
  */
  virtual bool isAbsorbing(long st) const;

private:
  reachgraph* RGR;

// Options
private:
  static long max_arc_display;
  static int graph_display_style;
  static bool display_graph_node_names;

  friend class init_graphllm;
};

#endif

