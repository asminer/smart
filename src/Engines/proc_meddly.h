
// $Id$

// Neat trick: both minimalist, and detailed, header information here.

#ifndef PROC_MEDDLY_H
#define PROC_MEDDLY_H

// Minimalist front-end for process generation, using Meddly

class exprman;

/// Should be called BEFORE any other Meddly-based initializer
void InitializeProcGenMeddly(exprman* em);

#endif

#ifdef PROC_MEDDLY_DETAILS
#ifndef PROC_MEDDLY_DETAIL_H
#define PROC_MEDDLY_DETAIL_H

//
// Detailed front-end for Meddly process generation
//

#include "gen_rg_base.h"
#include "../Modules/glue_meddly.h"

class model_event;
class dsde_hlm;
class meddly_states;
class radio_button;

// **************************************************************************
// *                                                                        *
// *                           minterm_pool class                           *
// *                                                                        *
// **************************************************************************

/** Collection of minterms.
    In particular this allows us to "share" minterms
    and not worry about deleting them.
    We use some extra ints per midterm (at the end) to pull this off:
    minterm[length]   gives the incoming count, if positive;
                      otherwise it is the negative of the
                      index of the next minterm in the free list.
    minterm[length+1] gives the index of this minterm.
    Trick: the zeroth minterm is always null.
*/
class minterm_pool {
  int** minterms;
  int alloc;
  int used;
  int free_list;  
  int term_depth;
protected:
  int* allocMinterm();
public:
  minterm_pool(int max_minterms, int depth);
  ~minterm_pool();
  
  inline int* shareMinterm(int* m) {
    DCASSERT(m);
    DCASSERT(m[term_depth]>0);
    m[term_depth]++;
    return m;
  }

  inline int* getMinterm() {
    if (0==free_list) return allocMinterm();
    int* answer = minterms[free_list];
    DCASSERT(answer[term_depth]<=0);
    free_list = -answer[term_depth];
    answer[term_depth] = 1;
    return answer;
  }

  inline void doneMinterm(int* m) {
    DCASSERT(m);
    DCASSERT(m[term_depth]>0);
    if (--m[term_depth] > 0) return;
    m[term_depth] = -free_list;
    free_list = m[term_depth+1];
  }

  inline void fillMinterm(int* dest, const int* src) const {
    memcpy(dest, src, term_depth * sizeof(int));
  }

  inline void showMinterm(OutputStream &s, const int* m) const {
    s.Put('[');
    s.PutArray(m, term_depth);
    s.Put(']');
  }

  inline bool equalMinterms(const int* m1, const int* m2) const {
    return 0==memcmp(m1+1, m2+1, (term_depth-1) * sizeof(int));
  }

  void reportStats(DisplayStream &out) const;
};

// **************************************************************************
// *                                                                        *
// *                         meddly_varoption class                         *
// *                                                                        *
// **************************************************************************

/** Variable options for Meddly.
    Abstracts away different state variable encoding and exploration 
    algorithms.
*/
class meddly_varoption {
  // Meddly variables: named?
  static bool vars_named;

  friend void InitializeProcGenMeddly(exprman* em);
protected:
  bool built_ok; 
  MEDDLY::dd_edge** event_enabling;
  MEDDLY::dd_edge** event_firing;
public:
  meddly_states &ms;
  const dsde_hlm &parent;
public:
  meddly_varoption(meddly_states &x, const dsde_hlm &p);
  virtual ~meddly_varoption();

  inline bool wasBuiltOK() const { return built_ok; }

  /// Build initial states and other initializations.
  virtual void initializeVars();

protected:
  static char* buildVarName(const hldsm::partinfo &part, int k);

public:
  /// Any pre-processing for the next-state function goes here.
  virtual void initializeEvents(named_msg &debug) = 0;

  inline const MEDDLY::dd_edge& getEventEnabling(int ev_index) const {
    DCASSERT(event_enabling);
    DCASSERT(event_enabling[ev_index]);
    return *event_enabling[ev_index];
  }

  inline const MEDDLY::dd_edge& getEventFiring(int ev_index) const {
    DCASSERT(event_firing);
    DCASSERT(event_firing[ev_index]);
    return *event_firing[ev_index];
  }

  /** Update the enabling and firing functions for all events.
      The update is driven by which levels have "changed".
        @param    d     Stream for debug info.
        @param    nsf   Encoder to use.
        @param    cl    0, or array of dimension number of MDD levels;
                        cl[i] is true if we need to reconsider
                        level i.  If the array is 0 then we
                        assume cl[i] is true for all i.

        @throws   An appropriate error code.
  */
  virtual void updateEvents(named_msg &d, meddly_encoder* nsf, bool* cl) = 0;
   
  /// For the given set, determine which levels have "changed".
  virtual bool hasChangedLevels(const MEDDLY::dd_edge &s, bool* cl) = 0;


  /// Show stats on completion
  virtual void reportStats(DisplayStream &s) const;

};


// **************************************************************************
// *                                                                        *
// *                          meddly_procgen class                          *
// *                                                                        *
// **************************************************************************


/** Process construction using Meddly.
    
    I.e., different algorithms to build the reachability set 
    and such are derived from this class.
*/
class meddly_procgen : public process_generator {
  // storage option
  static int proc_storage;
  static const int EVMXD = 0;
  static const int MTMXD = 1;

  // style option
  static int edge_style;
  static const int ACTUAL = 0;
  static const int POTENTIAL = 1;

  // Variable option
  static int var_type;
  static const int BOUNDED    = 0;
  static const int EXPANDING  = 1;
  static const int ON_THE_FLY = 2;
  static const int PREGEN     = 3;

  // node deletion policy options
  static int nsf_ndp;
  static int rss_ndp;
  static const int NEVER        = 0;
  static const int OPTIMISTIC   = 1;
  static const int PESSIMISTIC  = 2;

  friend radio_button** makeNDPButtons(int &num_buttons);
  friend void InitializeProcGenMeddly(exprman* em);
public:
  meddly_procgen();
  virtual ~meddly_procgen();

  inline static const char* getStyleName() {
    switch (var_type) {
      case BOUNDED:     return "bounded";
      case EXPANDING:   return "expanding";
      case ON_THE_FLY:  return "on-the-fly";
      case PREGEN:      return "pregen";
      default:          return "unknown";
    };
    return 0; // keep compilers happy
  }

  virtual MEDDLY::forest::policies buildNSFPolicies() const;
  virtual MEDDLY::forest::policies buildRSSPolicies() const;

protected:
  inline static bool useActualEdges() {
    return ACTUAL == edge_style;
  }
  inline static bool usePotentialEdges() {
    return POTENTIAL == edge_style;
  }

  inline static bool useEVMXD() {
    return EVMXD == proc_storage;
  }
  inline static bool useMTMXD() {
    return MTMXD == proc_storage;
  }

  /** Build a variable option class, according to option MeddlyVariables.
        @param  ms  meddly_states object, for final result (shared).
        @return     A new object of type meddly_varoption, or 0 on error.
  */
  inline meddly_varoption* 
  makeVariableOption(const dsde_hlm &m, meddly_states &ms) const {
    switch (var_type) {
      case BOUNDED:     return makeBounded(m, ms);
      case EXPANDING:   return makeExpanding(m, ms);
      case ON_THE_FLY:  return makeOnTheFly(m, ms);
      case PREGEN:      return makePregen(m, ms);
    }
    return 0;
  }
  
private:
  meddly_varoption* makeBounded(const dsde_hlm &m, meddly_states &ms) const;
  meddly_varoption* makeExpanding(const dsde_hlm &m, meddly_states &ms) const;
  meddly_varoption* makeOnTheFly(const dsde_hlm &m, meddly_states &ms) const;
  meddly_varoption* makePregen(const dsde_hlm &m, meddly_states &ms) const;
};

// TBD

#endif
#endif
