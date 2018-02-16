
#include "../Modules/glue_meddly.h"

#ifdef HAVE_MEDDLY_H

#include "satur_meddly.h"

#include "gen_meddly.h"

#include "../Options/options.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/engine.h"
#include "../ExprLib/sets.h"

#include "../Formlsms/dsde_hlm.h"
#include "../Formlsms/rss_meddly.h"
#include "../Formlsms/rgr_meddly.h"
#include "../Formlsms/proc_meddly.h"

#include "../Modules/biginttype.h"

#include "../_Timer/timerlib.h"
#include "../_LSLib/lslib.h"
#include "meddly_expert.h"
#include <iostream>
#include <vector>

#if 1
#include <unordered_map>
typedef std::unordered_map< MEDDLY::node_handle, bigint > NodeIntMap;
typedef std::unordered_map< MEDDLY::node_handle, NodeIntMap > NodeNodeIntMap;
#else
#include <map>
typedef std::map< MEDDLY::node_handle, bigint > NodeIntMap;
typedef std::map< MEDDLY::node_handle, NodeIntMap > NodeNodeIntMap;
#endif

#include "../ParseSM/parse_sm.h"
extern parse_module* pm;

// #define DEBUG_DETAILS
// #define DEBUG_DEPENDENCIES
// #define DEBUG_RADIX_SORT
// #define DEBUG_REFCOUNTS
// #define DEBUG_EVENT_NSFS
// #define DEBUG_FINAL_NSF
// #define DEBUG_FINAL_CTMC
// #define DEBUG_MINTERM
// #define REPORT_INITIAL

// **************************************************************************
// *                                                                        *
// *                        meddly_fsm_finish  class                        *
// *                                                                        *
// **************************************************************************

/** Neat trick:
    Special-purpose engine for finishing FSM construction, using meddly.
*/
class mxd_fsm_finish : public process_generator {
  bool potential;
  meddly_varoption* mvo;
  shared_ddedge* NSF;
public:
  mxd_fsm_finish(bool pot, meddly_varoption* mvo, shared_ddedge* nsf);
  virtual ~mxd_fsm_finish();
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual void RunEngine(hldsm* m, result &statesonly);
};

mxd_fsm_finish::mxd_fsm_finish(bool pot, meddly_varoption* _mvo, shared_ddedge* nsf) 
: process_generator()
{
  potential = pot;
  mvo = _mvo;
  NSF = nsf;
}

mxd_fsm_finish::~mxd_fsm_finish()
{
  delete mvo;
  Delete(NSF);
}

bool mxd_fsm_finish::AppliesToModelType(hldsm::model_type mt) const
{
  return (hldsm::Asynch_Events == mt);
  // Not sure if we need this, actually...
}

void mxd_fsm_finish::RunEngine(hldsm* hm, result &states_only)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));
  DCASSERT(hm->GetProcessType() == lldsm::FSM);
  lldsm* lm = hm->GetProcess();
  DCASSERT(lm);
  subengine* e = lm->getCompletionEngine();
  if (0==e)                   return;
  if (states_only.getBool())  return;
  if (e!=this)                return e->RunEngine(hm, states_only);

  graph_lldsm* glm = smart_cast <graph_lldsm*> (lm);
  DCASSERT(glm);

  timer watch;
  if (Report().startReport()) {
    Report().report() << "Finishing FSM using Meddly\n";
    Report().report() << "\tUsing ";
    Report().report() << ( potential ? "potential edges\n" : "actual edges\n" );
    Report().stopIO();
  }

  meddly_monolithic_rg* rgr = pm->computeMinimumTrace()
    ? new meddly_monolithic_min_rg(0, mvo->shareMxdWrap())
    : new meddly_monolithic_rg(0, mvo->shareMxdWrap());
  rgr->setPotential(Share(NSF));
  if (!potential) rgr->scheduleConversionToActual();
  glm->setRGR( rgr );


  if (Report().startReport()) {
    Report().report() << "Finished  FSM using Meddly, took ";
    Report().report() << watch.elapsed_seconds() << " seconds\n";
    Report().stopIO();
  }

  lm->setCompletionEngine(0);
  delete this;
}

// **************************************************************************
// *                                                                        *
// *                         meddly_mc_finish class                         *
// *                                                                        *
// **************************************************************************

/** Neat trick:
    Special-purpose engine for finishing FSM construction, using meddly.
    TBD: ev*mdd vs mtmdd option
*/
class mxd_mc_finish : public process_generator {
  bool potential;
  meddly_varoption* mvo;
  shared_ddedge* NSF;
public:
  mxd_mc_finish(bool pot, meddly_varoption* mvo, shared_ddedge* nsf);
  virtual ~mxd_mc_finish();
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual void RunEngine(hldsm* m, result &statesonly);
};

mxd_mc_finish::mxd_mc_finish(bool pot, meddly_varoption* _mvo, shared_ddedge* nsf)
: process_generator()
{
  potential = pot;
  mvo = _mvo;
  NSF = nsf;
}

mxd_mc_finish::~mxd_mc_finish()
{
  delete mvo;
  Delete(NSF);
}

bool mxd_mc_finish::AppliesToModelType(hldsm::model_type mt) const
{
  return (hldsm::Asynch_Events == mt);
  // Not sure if we need this, actually...
}

void mxd_mc_finish::RunEngine(hldsm* hm, result &states_only)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));
  DCASSERT(hm->GetProcessType() == lldsm::CTMC);
  lldsm* lm = hm->GetProcess();
  DCASSERT(lm);
  subengine* e = lm->getCompletionEngine();
  if (0==e)                   return;
  if (states_only.getBool())  return;
  if (e!=this)                return e->RunEngine(hm, states_only);

  stochastic_lldsm* slm = smart_cast <stochastic_lldsm*> (lm);
  DCASSERT(slm);

  timer watch;
  if (Report().startReport()) {
    Report().report() << "Finishing CTMC using Meddly\n";
    Report().report() << "\tUsing ";
    Report().report() << ( potential ? "potential edges\n" : "actual edges\n" );
    Report().stopIO();
  }

  //
  // Set reachability graph
  //
  meddly_monolithic_rg* rgr = new meddly_monolithic_rg(0, mvo->shareMxdWrap());
  rgr->setPotential(Share(NSF));
  if (!potential) rgr->scheduleConversionToActual();
  slm->setRGR( rgr );

  //
  // Build and set CTMC
  //

  dsde_hlm* m = smart_cast <dsde_hlm*> (hm);
  DCASSERT(m);

  meddly_encoder* procmxd = rgr->newMxdWrapper("proc", MEDDLY::forest::REAL, 
    MEDDLY::forest::MULTI_TERMINAL);
  shared_ddedge* R = smart_cast<shared_ddedge*>(procmxd->makeEdge(0));
  DCASSERT(R);
  procmxd->buildSymbolicConst(0.0, R);

  //
  //  Add rates for each event to CTMC.
  //  
  traverse_data x(traverse_data::BuildDD);
  result ans;
  x.answer = &ans;
  MEDDLY::dd_edge tmp(procmxd->getForest());
  for (long e=0; e<m->getNumEvents(); e++) {
    // Get rate DD for this event
    x.ddlib = procmxd;
    x.which = traverse_data::BuildExpoRateDD;
    DCASSERT(m->readEvent(e));
    expr* distro = m->readEvent(e)->getDistribution();
    DCASSERT(distro);
    distro->Traverse(x);
    DCASSERT(ans.isNormal());
    shared_ddedge* r_e = Share(smart_cast<shared_ddedge*>(ans.getPtr()));
    DCASSERT(r_e);

    // Element-wise multiply by enabling & firing expressions
    // TBD: check for memory overflows
    MEDDLY::apply(MEDDLY::COPY, mvo->getEventEnabling(e), tmp);
    r_e->E *= tmp;
    MEDDLY::apply(MEDDLY::COPY, mvo->getEventFiring(e), tmp);
    r_e->E *= tmp;

    // Add to overall rate matrix
    R->E += r_e->E;

    // Cleanup
    Delete(r_e);
  } // for e

  //
  // Finalize process
  //

  meddly_process* PROC = new meddly_process(procmxd);
  PROC->setActual(R);
  // TBD!  NEED an actual initial vector!
  LS_Vector initial;
  slm->setPROC(initial, PROC);

  if (Report().startReport()) {
    Report().report() << "Finished  CTMC using Meddly, took ";
    Report().report() << watch.elapsed_seconds() << " seconds\n";
    Report().stopIO();
  }

  lm->setCompletionEngine(0);
  delete this;
}

// **************************************************************************
// *                                                                        *
// *                        meddly_implicitgen class                        *
// *                                                                        *
// **************************************************************************

/** Abstract base class for implicit process generation with meddly.
    Common stuff is implemented here :^)

*/
class meddly_implicitgen : public meddly_procgen {
  // TBD - change this for non-monolithic...
  shared_ddedge* NSF;
protected:
  static int order_policy;
  static const int ORDER_HIGH_TO_LOW = 0;
  static const int ORDER_LOW_TO_HIGH = 1;
  static const int ORDER_MODEL = 2;
  friend class init_saturmeddly;

  int* event_order;
  int event_order_size;
public:
  meddly_implicitgen();
  virtual ~meddly_implicitgen();
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual void RunEngine(hldsm* m, result &states_only); 

protected:
  inline const MEDDLY::dd_edge& getNSF() const {
    DCASSERT(NSF);
    return NSF->E;
  }

  inline void setNSF(shared_ddedge* nsf) {
    DCASSERT(nullptr == NSF);
    NSF = nsf;
  }

  virtual void generateRSS(meddly_varoption &x, timer &w) = 0;
  virtual const char* getAlgName() const = 0;

  // Called before we begin state generation
  virtual void initGen() { };

  // Called after we are done state generation
  virtual void doneGen() { };

  virtual void reportGen(bool err, DisplayStream &s) const { };

  void buildNextStateFunc(meddly_varoption &x);
  
  void preprocess(dsde_hlm &m);

  virtual void postprocess(dsde_hlm &m, meddly_varoption &x) { }

  inline static void checkTerm(const char* errstr, const hldsm &hm) {
    if (!em->caughtTerm()) return;
    if (hm.StartError(0)) {
      em->cerr() << "signal caught during " << errstr;
      hm.DoneError();
    }
    throw Terminated;
  }

  inline static void convert(MEDDLY::error ce, const char* errstr,
                              const hldsm &hm) 
  {
    if (hm.StartError(0)) {
      em->cerr() << errstr << " in Meddly with error code:";
      em->newLine();
      em->cerr() << ce.getName();
      hm.DoneError();
    }
    switch (ce.getCode()) {
      case MEDDLY::error::INSUFFICIENT_MEMORY:  throw  Out_Of_Memory;
      default:                                  throw  Engine_Failed;
    }
  }

  inline bool startGen(const hldsm &hm) const {
    if (!meddly_procgen::startGen(hm, "reachability set")) return false;
    Report().report() << " using Meddly: ";
    Report().report() << getAlgName() << " algorithm, ";
    Report().report() << getStyleName() << " vars.\n";
    return true;
  }

  inline bool stopGen(bool err, const hldsm &hm, 
                              const timer &w) const {
    return meddly_procgen::stopGen(err, hm, "reachability set", w);
  }

  virtual void buildRSS(meddly_varoption &x);

private:
  void radix_sort(const hldsm::partinfo &p, const dsde_hlm &m, int a, int b, int k, bool dec);
};

int meddly_implicitgen::order_policy;

// **************************************************************************
// *                       meddly_implicitgen methods                       *
// **************************************************************************

meddly_implicitgen::meddly_implicitgen() : meddly_procgen()
{
  event_order = 0;
  event_order_size = 0;
  NSF = 0;
}

meddly_implicitgen::~meddly_implicitgen()
{
  free(event_order);
  Delete(NSF);
}

bool meddly_implicitgen::AppliesToModelType(hldsm::model_type mt) const
{
  return (hldsm::Asynch_Events == mt);
}

void meddly_implicitgen::RunEngine(hldsm* hm, result &states_only)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));
  lldsm* lm = hm->GetProcess();
  if (lm) {
    // we already have something, deal with it
    subengine* e = lm->getCompletionEngine();
    if (0==e)                   return;
    if (states_only.getBool())  return;
    if (e!=this)                return e->RunEngine(hm, states_only);
    // Shouldn't get here
    DCASSERT(0);
    throw Engine_Failed;
  } 
  
  meddly_reachset* rss = 0;
  meddly_varoption* mvo = 0;
  try {
    dsde_hlm* dhm = smart_cast <dsde_hlm*> (hm);
    DCASSERT(dhm);
    preprocess(*dhm);
    initGen();
    rss = new meddly_reachset;
    mvo = makeVariableOption(*dhm, *rss);
    DCASSERT(mvo);
    buildRSS(*mvo);
    postprocess(*dhm, *mvo);
    doneGen();
  }
  catch (subengine::error e) {
    doneGen();
    Delete(rss);
    delete mvo;
    hm->SetProcess(MakeErrorModel());
    throw e;
  }

  state_lldsm* slm = 0;
  subengine* finisher = 0;
  if (hm->GetProcessType() == lldsm::FSM) {
    slm = new graph_lldsm(lldsm::FSM);
    finisher = new mxd_fsm_finish(usePotentialEdges(), mvo, Share(NSF));
  } else {
    slm = new stochastic_lldsm(hm->GetProcessType());
    finisher = new mxd_mc_finish(usePotentialEdges(), mvo, Share(NSF));
  }
  slm->setRSS(rss);
  hm->SetProcess(slm);
  slm->setCompletionEngine(finisher);

  if (states_only.getBool())  return;
  if (0==finisher)            throw No_Engine;

  finisher->RunEngine(hm, states_only);
}

void meddly_implicitgen::buildNextStateFunc(meddly_varoption &x)
{
  if (Debug().startReport()) {
    Debug().report() << "Updating event DDs\n";
    Debug().stopIO();
  }
  x.updateEvents(Debug(), 0);

  const dsde_hlm &m = x.getParent();
  shared_ddedge* N = smart_cast<shared_ddedge*>(x.make_mxd_constant(false));
  DCASSERT(N);

  for (long e=0; e<m.getNumEvents(); e++) {
    MEDDLY::dd_edge enable = x.getEventEnabling(e);

#ifdef DEBUG_DETAILS
    if (Debug().startReport()) {
      Debug().report() << "Enabling DD for event ";
      Debug().report() << m.readEvent(e)->Name();
      Debug().report() << " DD edge: " << enable.getNode() << "\n";
      Debug().report().flush();
      smart_output Drep(Debug().report());
      enable.show(Drep, 2);
      Debug().stopIO();
    }
#endif

    MEDDLY::dd_edge firing = x.getEventFiring(e);

#ifdef DEBUG_DETAILS
    if (Debug().startReport()) {
      Debug().report() << "Next-state DD for event ";
      Debug().report() << m.readEvent(e)->Name();
      Debug().report() << " DD edge: " << firing.getNode() << "\n";
      Debug().report().flush();
      smart_output Drep(Debug().report());
      firing.show(Drep, 2);
      Debug().stopIO();
    }
#endif

    // TBD: deal with priority

    // "AND" together the enabling and next state.
    firing *= enable;

#ifdef DEBUG_EVENT_NSFS
    if (Debug().startReport()) {
      Debug().report() << "(final) next-state DD for event ";
      Debug().report() << m.readEvent(e)->Name();
      Debug().report() << " DD edge: " << firing.getNode() << "\n";
      Debug().report().flush();
      smart_output Drep(Debug().report());
      firing.show(Drep, 2);
      Debug().stopIO();
    }
#endif

    // Add this to overall next-state function  
    N->E += firing;
  } // for e
 
  Delete(NSF);
  NSF = N;
}


void meddly_implicitgen::preprocess(dsde_hlm &m) 
{
  // Check partition 
  if (!m.buildPartInfo()) {
    if (m.StartError(0)) {
      em->cerr() << "Saturation requires a structured model (try partitioning)";
      m.DoneError();
    }
    throw Engine_Failed;
  }

  // Build event dependencies
  const hldsm::partinfo &part = m.getPartInfo();
  for (int e=0; e<m.getNumEvents(); e++) {
    DCASSERT(m.readEvent(e));
    m.getEvent(e)->buildEnablingDependencies(part.num_levels, part.num_vars);
    m.getEvent(e)->buildNextstateDependencies(part.num_levels, part.num_vars);
#ifdef DEBUG_DEPENDENCIES
    em->cout() << "Event " << m.readEvent(e)->Name();
    em->cout() << " depends on levels:\n\t";
    for (int z=0; z<=part.num_levels; z++)
      if (m.readEvent(e)->dependsOnLevel(z))
        em->cout() << z << " ";
    em->cout() << "\n";
    em->cout() << "\tdepends on variables:\n\t";
    for (int z=0; z<part.num_vars; z++)
      if (m.readEvent(e)->dependsOnVar(z))
        em->cout() << m.readStateVar(z)->Name() << " ";
    em->cout() << "\n";
    em->cout().flush();
#endif
  }

  // Enlarge event ordering array, if necessary
  if (event_order_size < m.getNumEvents()) {
    DCASSERT(part.num_levels>0);
    int* foo = (int*) realloc(event_order, m.getNumEvents() * sizeof(int));
    if (0==foo) {
      if (m.StartError(0)) {
        em->cerr() << "Not enough memory for event ordering";
        m.DoneError();
      }
      throw Out_Of_Memory;
    }
    event_order = foo;
    event_order_size = m.getNumEvents();
  }

  // Put events in order for next-state construction
  for (int e=0; e<m.getNumEvents(); e++) {
    event_order[e] = e;
  }
  switch (order_policy) {
    case ORDER_LOW_TO_HIGH:
      radix_sort(part, m, 0, m.getNumEvents(), part.num_levels, false);
      break;

    case ORDER_HIGH_TO_LOW:
      radix_sort(part, m, 0, m.getNumEvents(), part.num_levels, true);
      break;

    case ORDER_MODEL:
      break;

    default:
      if (em->startInternal(__FILE__, __LINE__)) {
        em->causedBy(0);
        em->internal() << "Bad value for order policy: " << order_policy;
        em->stopIO();
      };
      // shouldn't get here
      throw Engine_Failed;
  };

#ifdef DEBUG_DEPENDENCIES
  em->cout() << "Using event order:\n\t" << event_order[0]->Name();
  for (int e=1; e<m.getNumEvents(); e++) {
    em->cout() << ", " << event_order[e]->Name();
  }
  em->cout() << "\n";
#endif
}

void meddly_implicitgen
::radix_sort(const hldsm::partinfo &part, const dsde_hlm &m, 
              int a, int b, int k, bool dec)
{
  if (a+1==b || a==b || 0==k) return;
  DCASSERT(a<b);
#ifdef DEBUG_RADIX_SORT  
  em->cout() << "Radix Sort bit " << k << ": ";
  for (int e=a; e<b; e++) 
    em->cout() << m.readEvent(event_order[e])->Name() << " ";
  em->cout() << "\n";
#endif
  // sort based on bit k.
  //
  // front of array holds entries with bit k == dec.
  // back  of array holds entries with bit k != dec.
  //

  int ap = a;
  int bp = b-1;

  // Determine front pointer, first time
  while (ap < b) {
    if (m.readEvent(event_order[ap])->dependsOnLevel(k) != dec) break;
    ap++;
  }
  if (ap >= b-1) {
    radix_sort(part, m, a, ap, k-1, dec);
    return;
  }

  for (;;) {

    // find smallest entry != dec
    while (m.readEvent(event_order[ap])->dependsOnLevel(k) == dec) {
      ap++;
      if (ap >= bp) {
        radix_sort(part, m, a, ap, k-1, dec);
        radix_sort(part, m, ap, b, k-1, dec);
        return;
      }
    } // while

    // find largest entry == dec
    while (m.readEvent(event_order[bp])->dependsOnLevel(k) != dec) {
      bp--;
      if (ap >= bp) {
        radix_sort(part, m, a, ap, k-1, dec);
        radix_sort(part, m, ap, b, k-1, dec);
        return;
      }
    } // while

#ifdef DEBUG_RADIX_SORT  
    em->cout() << "SWAP " << m.readEvent(event_order[ap])->Name() << " ";
    em->cout() << m.readEvent(event_order[bp])->Name() << "\n";
#endif

    SWAP(event_order[ap], event_order[bp]);
    
  } // infinite loop
}


void meddly_implicitgen::buildRSS(meddly_varoption &x)
{
  timer watch;
  timer subwatch;
  if (startGen(x.getParent())) {
    Report().stopIO();
  }

  //
  // Build the initial state set, and other initializations
  //
  if (Report().startReport()) {
    Report().report() << "Initializing forests\n";
    Report().stopIO();
  }

  try {
    x.initializeVars();

    if (Report().startReport()) {
      Report().report() << "Initialized  forests, took ";
      Report().report() << watch.elapsed_seconds() << " seconds\n";
      Report().stopIO();
    }

    x.initializeEvents(Debug());

    // 
    // Build next-state function
    //
    if (Report().startReport()) {
      Report().report() << "Building next-state function\n";
      subwatch.reset();
      Report().stopIO();
    }

    buildNextStateFunc(x);

    if (Report().startReport()) {
      Report().report() << "Built    next-state function, took ";
      Report().report() << subwatch.elapsed_seconds() << " seconds\n";
  #ifdef DEBUG_FINAL_NSF
      Report().report() << "DD edge: " << getNSF().getNode() << "\n";
      Report().report().flush();
      smart_output Rrep(Report().report());
      getNSF().show(Rrep, 2);
      Report().report() << "Initial state: " << x.getInitial().getNode() << "\n";
      Report().report().flush();
      x.getInitial().show(Rrep, 2);
  #endif
  #ifdef DEBUG_REFCOUNTS
      Report().report() << "Forest:\n";
      Report().report().flush();
      getNSF().getForest()->showInfo(Rrep, 1);
      fflush(Report().Freport());
  #endif
      Report().stopIO();
    }

    //
    // Generate reachability set
    //
    if (Report().startReport()) {
      Report().report() << "Building reachability set\n";
      Report().stopIO();
      subwatch.reset();
    }

    generateRSS(x, subwatch);

    if (Report().startReport()) {
      Report().report() << "Built    reachability set, took ";
      Report().report() << subwatch.elapsed_seconds() << " seconds\n";
      Report().stopIO();
    }

    if (stopGen(false, x.getParent(), watch)) {
      reportGen(false, Report().report());
      x.reportStats(Report().report());
      Report().stopIO();
    }
  } // try

  catch (subengine::error status) {
    if (stopGen(true, x.getParent(), watch)) Report().stopIO();
    throw status;
  }
}

// **************************************************************************
// *                                                                        *
// *                        meddly_saturation  class                        *
// *                                                                        *
// **************************************************************************

/** Saturation using Meddly.
*/
class meddly_saturation : public meddly_implicitgen {
public:
  meddly_saturation();
  virtual ~meddly_saturation();
protected:
  virtual void generateRSS(meddly_varoption &x, timer &w);
  virtual const char* getAlgName() const { return "saturation"; }
};

meddly_saturation the_meddly_saturation;

// **************************************************************************
// *                       meddly_saturation  methods                       *
// **************************************************************************

meddly_saturation::meddly_saturation() : meddly_implicitgen()
{
}

meddly_saturation::~meddly_saturation()
{
}

void meddly_saturation::generateRSS(meddly_varoption &x, timer&)
{
  try {
    shared_ddedge* S = x.newMddEdge();
    MEDDLY::apply(
      MEDDLY::REACHABLE_STATES_DFS,
      x.getInitial(), 
      getNSF(),
      S->E
    );
    x.setStates(S);
    checkTerm("Generation failed", x.getParent());
  }
  catch (MEDDLY::error ce) {
    convert(ce, "Generation failed", x.getParent());
  }
}

// **************************************************************************
// *                                                                        *
// *                          meddly_otfsat  class                          *
// *                                                                        *
// **************************************************************************

/** On-the-fly saturation using Meddly.

    HACK!  
    RE-DESIGN THIS CLASS AND EVERYTHING ELSE IN HERE!
    IT'S ALL CRAP!

*/
class meddly_otfsat : public meddly_implicitgen {
public:
  meddly_otfsat();
  virtual ~meddly_otfsat();
protected:
  virtual void buildRSS(meddly_varoption &x);
  virtual void generateRSS(meddly_varoption &x, timer &w) {
    DCASSERT(0);
  }
  virtual const char* getAlgName() const { return "on the fly saturation"; }
  virtual void postprocess(dsde_hlm &m, meddly_varoption &x);
private:
  // Build the otf next-state function
  //
  MEDDLY::satotf_opname::otf_relation*  buildNSF(meddly_varoption &x);

  // Build the reachable set of states
  //
  void generateRSS(meddly_varoption &x, MEDDLY::satotf_opname::otf_relation* NSF);

  // Clear Meddly's compute tables
  //
  void clearMeddlyComputeTable(meddly_varoption &x,
      MEDDLY::satotf_opname::otf_relation &NSF);

  // Compute the maximum number of tokens that can occur at any place in the Petri Net
  //
  long computeMaxTokensInPlace(meddly_varoption &x,
      MEDDLY::satotf_opname::otf_relation &NSF);

  // Compute the number of arcs in the reachability graph
  //
  bigint computeNumTransitions(const MEDDLY::dd_edge &RS,
      MEDDLY::satotf_opname::otf_relation &NSF, bool count_duplicates);

  // Compute the maximum number of tokens in any state of the reachable set of states
  //
  bool computeMaxTokensPerLevel(
      meddly_varoption &x,
      MEDDLY::satotf_opname::otf_relation &NSF,
      std::vector<long> &result,
      std::vector< std::vector<long> > &level_index_to_token);
  long computeMaxTokensPerMarking(
      const std::vector<long> &level_to_max_tokens,
      const std::vector< std::vector<long> > &level_index_to_token,
      MEDDLY::expert_forest* mddf,
      MEDDLY::node_handle mdd,
      int level, 
      std::unordered_map < MEDDLY::node_handle, long > &ct);
  long computeMaxTokensPerMarking(
      meddly_varoption &x,
      MEDDLY::satotf_opname::otf_relation &NSF);
};

meddly_otfsat the_meddly_otfsat;

meddly_otfsat::meddly_otfsat() : meddly_implicitgen()
{
}

meddly_otfsat::~meddly_otfsat()
{
}

long meddly_otfsat::computeMaxTokensInPlace(
    meddly_varoption &x,
    MEDDLY::satotf_opname::otf_relation &NSF)
{
  substate_colls* colls = x.getSubstateStorage();
  if (0 == colls) return -1;

  const dsde_hlm& p = x.getParent();
  shared_state* foo = new shared_state(&p);
  long max_tokens = 0;
  for (int k = 1; k <= p.getPartInfo().num_levels; k++) {
    long nConfirmed = NSF.getNumConfirmed(k);
    int sz = foo->readSubstateSize(k);
    int chunk[sz];
    int curr = 0;
    for (int i = 0; i < nConfirmed; i++, curr++) {
      while (!NSF.isConfirmed(k, curr)) curr++;
       //std::cout << "k: " << k << ", i: " << i << ", curr: " << curr << ", sz:" << sz << "\n";
      colls->getSubstate(k, curr, chunk, sz);
      for (int j = 0; j < sz; j++) {
        //std::cout << "\t\t chunk[" << j << "]: " << chunk[j] << "\n";
        if (max_tokens < chunk[j]) max_tokens = chunk[j];
      }
    }
  }
  return max_tokens;
}

bool meddly_otfsat::computeMaxTokensPerLevel(
    meddly_varoption &x,
    MEDDLY::satotf_opname::otf_relation &NSF,
    std::vector<long> &max_tokens,
    std::vector< std::vector<long> > &level_index_to_token)
{
  substate_colls* colls = x.getSubstateStorage();
  if (0 == colls) return false;

  const dsde_hlm& p = x.getParent();
  shared_state* foo = new shared_state(&p);

  max_tokens.resize(p.getPartInfo().num_levels+1);
  for (std::vector<long>::iterator rIter = max_tokens.begin();
      rIter != max_tokens.end(); rIter++) {
    *rIter = 0;
  }

  level_index_to_token.resize(p.getPartInfo().num_levels+1);
  for (int i = 0; i < int(level_index_to_token.size()); i++) {
    for (std::vector<long>::iterator rIter = level_index_to_token[i].begin();
        rIter != level_index_to_token[i].end(); rIter++) {
      *rIter = 0;
    }
  }

  for (int k = 1; k <= p.getPartInfo().num_levels; k++) {
    long nConfirmed = NSF.getNumConfirmed(k);
    int sz = foo->readSubstateSize(k);
    int chunk[sz];
    int curr = 0;
    for (int i = 0; i < nConfirmed; i++, curr++) {
      //std::cout << "k: " << k << ", i: " << i << ", curr: " << curr << ", sz:" << sz << "\n";
      while (!NSF.isConfirmed(k, curr)) curr++;
      level_index_to_token[k].resize(curr+1, 0);
      colls->getSubstate(k, curr, chunk, sz);
      int sum = 0;
      for (int j = 0; j < sz; j++) {
        //std::cout << "\t\t chunk[" << j << "]: " << chunk[j] << "\n";
        sum += chunk[j];
      }
      level_index_to_token[k][curr] = sum;
      if (max_tokens[k] < sum) max_tokens[k] = sum;
      //std::cout << "\t\t max: " << max_tokens[k] << "\n";
    }
  }

  return true;
}


long meddly_otfsat::computeMaxTokensPerMarking(
    const std::vector<long> &level_to_max_tokens,
    const std::vector< std::vector<long> > &level_index_to_token,
    MEDDLY::expert_forest* mddf,
    MEDDLY::node_handle mdd,
    int level, 
    std::unordered_map < MEDDLY::node_handle, long > &ct)
{
  DCASSERT(0 != mdd);
  if (0 == level) return 0;

  int mddLevel = mddf->getNodeLevel(mdd);
  long result = 0;

  if (mddLevel < level) {
    // if mddLevel < level
    // --- level was fully skipped, return level_size * compute(mdd, mxd, level-1)
    result = level_to_max_tokens[level] +
      computeMaxTokensPerMarking(level_to_max_tokens, level_index_to_token,
          mddf, mdd, level-1, ct);
  } else {
    // check compute table
    std::unordered_map < MEDDLY::node_handle, long >::iterator iter = ct.find(mdd);
    if (iter != ct.end()) {
      return iter->second;
    }

    // expand mdd
    MEDDLY::unpacked_node* mdd_nr = MEDDLY::unpacked_node::newFromNode(mddf, mdd, false);
    for (int i = 0; i < mdd_nr->getNNZs(); i++) {
      result = MAX( result,
          (level_index_to_token[level][mdd_nr->i(i)] +
           computeMaxTokensPerMarking(level_to_max_tokens, level_index_to_token,
             mddf, mdd_nr->d(i), level-1, ct)) );
    }
    MEDDLY::unpacked_node::recycle(mdd_nr);

    // insert into compute table
    ct[mdd] = result;
  }

  return result;
}

long meddly_otfsat::computeMaxTokensPerMarking(
    meddly_varoption &x,
    MEDDLY::satotf_opname::otf_relation &NSF)
{
  // traverse the set of reachable states
  //
  // for all children node,
  //    max = MAX(max, map_i_to_token(level, i) + max_marking(child[i], level-1))

  const MEDDLY::dd_edge& states = x.getStates();
  if (0 == states.getNode()) return 0;

  std::vector<long> level_to_max_tokens;
  std::vector< std::vector<long> > level_index_to_token;
  if (!computeMaxTokensPerLevel(x, NSF, level_to_max_tokens, level_index_to_token))
    return -1;
  //for (int i = 0; i < int(level_to_max_tokens.size()); i++) {
  //  em->cout() << "Level: " << i << ", max tokens: " << level_to_max_tokens[i] << "\n";
  //}

  std::unordered_map < MEDDLY::node_handle, long > ct;
  return computeMaxTokensPerMarking(
      level_to_max_tokens,
      level_index_to_token,
      smart_cast<MEDDLY::expert_forest*>(states.getForest()),
      states.getNode(),
      x.getParent().getPartInfo().num_levels,
      ct);
}


bigint meddly_otfsat::computeNumTransitions(
    const MEDDLY::dd_edge &RS,
    MEDDLY::satotf_opname::otf_relation &NSF,
    bool count_duplicates)
{
  // num_trans = 0
  // for each event e in NSF,
  //    num_trans += computeNumTransitions(rs, e)

  int num_vars = RS.getForest()->getDomain()->getNumVariables();
  MEDDLY::dd_edge mxd_mask(NSF.getRelForest());
  MEDDLY::apply(MEDDLY::CROSS, RS, RS, mxd_mask);

  bigint num_transitions = 0l;

  if (count_duplicates) {
    for (int i = 1; i <= num_vars; i++) {
      for (int ei = 0; ei < NSF.getNumOfEvents(i); ei++) {
        bigint card = 0l;
        MEDDLY::dd_edge event_ei(NSF.getRelForest());
        event_ei.set(NSF.getRelForest()->linkNode(NSF.getEvent(i, ei)));
        event_ei *= mxd_mask;
        MEDDLY::apply(MEDDLY::CARDINALITY, event_ei, card.value);
        num_transitions.add(num_transitions, card);
      }
    }
  } else {
    // build monolithic 
    MEDDLY::dd_edge monolithic_nsf(NSF.getRelForest());
    for (int i = 1; i <= num_vars; i++) {
      MEDDLY::dd_edge nsf_i(NSF.getRelForest());
      for (int ei = 0; ei < NSF.getNumOfEvents(i); ei++) {
        MEDDLY::dd_edge event_ei(NSF.getRelForest());
        event_ei.set(NSF.getRelForest()->linkNode(NSF.getEvent(i, ei)));
        nsf_i += event_ei;
      }
      monolithic_nsf += nsf_i;
    }
    monolithic_nsf *= mxd_mask;
    MEDDLY::apply(MEDDLY::CARDINALITY, monolithic_nsf, num_transitions.value);
  }

  return num_transitions;
}


void meddly_otfsat::clearMeddlyComputeTable(
    meddly_varoption &x,
    MEDDLY::satotf_opname::otf_relation &NSF)
{
  NSF.clearMinterms();
  MEDDLY::operation::removeAllFromMonolithic();
}


void meddly_otfsat::buildRSS(meddly_varoption &x)
{
  timer watch;
  timer subwatch;
  if (startGen(x.getParent())) {
    Report().stopIO();
  }

  //
  // Build the initial state set, and other initializations
  //
  if (Report().startReport()) {
    Report().report() << "Initializing forests\n";
    Report().stopIO();
  }

  try {
    x.initializeVars();

    if (Report().startReport()) {
      Report().report() << "Initialized  forests, took ";
      Report().report() << watch.elapsed_seconds() << " seconds\n";
      Report().stopIO();
    }

    x.initializeEvents(Debug());

    // 
    // Build next-state function
    //
    if (Report().startReport()) {
      Report().report() << "Initializing next-state function builder\n";
      subwatch.reset();
      Report().stopIO();
    }

    MEDDLY::satotf_opname::otf_relation* NSF = buildNSF(x);
    DCASSERT(NSF);

    NSF->confirm(x.getInitial());

    if (Report().startReport()) {
      Report().report() << "Initialized  next-state function builder, took ";
      Report().report() << subwatch.elapsed_seconds() << " seconds\n";
      Report().stopIO();
    }

    //
    // Generate reachability set
    //
    if (Report().startReport()) {
      Report().report() << "Building reachability set\n";
      Report().stopIO();
      subwatch.reset();
    }

    generateRSS(x, NSF);

    if (Report().startReport()) {
      Report().report() << "Built    reachability set, took ";
      Report().report() << subwatch.elapsed_seconds() << " seconds\n";
      Report().stopIO();
    }

    if (stopGen(false, x.getParent(), watch)) {
      reportGen(false, Report().report());
      x.reportStats(Report().report());
      Report().report() << "\tMinterms:\t" << NSF->mintermMemoryUsage() << "  bytes\n";
      Report().stopIO();
    }

    clearMeddlyComputeTable(x, *NSF);

    result numstates;
    x.getNumStates(numstates);
    if (!numstates.isNormal()) {
      //
      // TBD: Error, can we print something and exit cleanly here?
      em->cout() << "CANNOT COMPUTE\n";
      delete NSF;
      return;
    }

#if 0
    // Build a monolithic transition relation
    NSF->bindExtensibleVariables();
    MEDDLY::node_handle mxd = NSF->getBoundedMonolithicNSF();
    shared_ddedge* d = new shared_ddedge(NSF->getRelForest());
    d->E.set(mxd);
    setNSF(d);

    clearMeddlyComputeTable(x, *NSF);
#else
    em->cout() << "STATE_SPACE STATES ";
    shared_object* bigns = numstates.getPtr();
    if (bigns) {
      bigns->Print(em->cout(), 0);
    } else {
      long ns = numstates.getInt();
      em->cout() << ns;
    }
    em->cout() << " TECHNIQUES SEQUENTIAL_PROCESSING DECISION_DIAGRAMS\n";
    em->cout().flush();

    //
    // Compute maximum tokens in any place
    //
    long max_tokens_in_place = computeMaxTokensInPlace(x, *NSF);
    em->cout() << "STATE_SPACE MAX_TOKEN_IN_PLACE " << max_tokens_in_place;
    em->cout() << " TECHNIQUES SEQUENTIAL_PROCESSING DECISION_DIAGRAMS\n";
    em->cout().flush();

    //
    // Compute the maximum number of tokens in a marking
    //
    long max_tokens_per_marking = computeMaxTokensPerMarking(x, *NSF);
    em->cout() << "STATE_SPACE MAX_TOKEN_PER_MARKING " << max_tokens_per_marking;
    em->cout() << " TECHNIQUES SEQUENTIAL_PROCESSING DECISION_DIAGRAMS\n";
    em->cout().flush();

    // Build a monolithic transition relation
    NSF->bindExtensibleVariables();

    //
    // Compute number of arcs
    //
#if 1
    em->cout() << "STATE_SPACE TRANSITIONS ";
    bigint num_dup_transitions = computeNumTransitions(x.getStates(), *NSF, true);
    num_dup_transitions.Print(em->cout(), 0);
    em->cout() << " TECHNIQUES SEQUENTIAL_PROCESSING DECISION_DIAGRAMS\n";
    em->cout().flush();

    em->cout() << "STATE_SPACE UNIQUE TRANSITIONS ";
    bigint num_transitions = computeNumTransitions(x.getStates(), *NSF, false);
    num_transitions.Print(em->cout(), 0);
    em->cout() << " TECHNIQUES SEQUENTIAL_PROCESSING DECISION_DIAGRAMS\n";
    em->cout().flush();
#else
    const MEDDLY::dd_edge& states = x.getStates();
    double num_transitions = NSF->getArcCount(states, true);
    em->cout() << "STATE_SPACE TRANSITIONS ";
    em->cout() << num_transitions;
    em->cout() << " TECHNIQUES SEQUENTIAL_PROCESSING DECISION_DIAGRAMS\n";
    num_transitions = NSF->getArcCount(states, false);
    em->cout() << "STATE_SPACE UNIQUE TRANSITIONS ";
    em->cout() << num_transitions;
    em->cout() << " TECHNIQUES SEQUENTIAL_PROCESSING DECISION_DIAGRAMS\n";
    em->cout().flush();
#endif

    MEDDLY::node_handle mxd = NSF->getBoundedMonolithicNSF();
    shared_ddedge* d = new shared_ddedge(NSF->getRelForest());
    d->E.set(mxd);
    setNSF(d);

    clearMeddlyComputeTable(x, *NSF);
#endif

    delete NSF;
    
  } // try

  catch (subengine::error status) {
    if (stopGen(true, x.getParent(), watch)) Report().stopIO();
    throw status;
  }
}

void meddly_otfsat::postprocess(dsde_hlm &m, meddly_varoption &x)
{
  // Update variable bounds
  const MEDDLY::domain* d = x.getMddForest()->getDomain();
  int num_vars = m.getNumStateVars();
  for (int i = 0; i < num_vars; i++) {
    model_statevar* var = m.getStateVar(i);
    if (!var->HasBounds()) {
      int level = var->GetPart();
      // The bound of primed variable
      var->SetBounds(MakeRangeSet(0, d->getVariableBound(level, true), 1));
    }
  }
}

MEDDLY::satotf_opname::otf_relation*  
meddly_otfsat::buildNSF(meddly_varoption &x)
{
  // FOR NOW!
  // TBD!

  return x.buildNSF_OTF(Debug());
}

void meddly_otfsat::generateRSS(meddly_varoption &x, 
    MEDDLY::satotf_opname::otf_relation* NSF)
{
  using namespace MEDDLY;

  DCASSERT(NSF);
  DCASSERT(MEDDLY::SATURATION_OTF_FORWARD);

  try {
    specialized_operation* satop = SATURATION_OTF_FORWARD->buildOperation(NSF);
    DCASSERT(satop);

    shared_ddedge* S = x.newMddEdge();
    satop->compute(x.getInitial(), S->E);

    // TBD - reindex?
    // TBD - grab NSF for model checking
    // TBD - after that, delete NSF?

    x.setStates(S);
    checkTerm("Generation failed", x.getParent());
  }
  catch (MEDDLY::error ce) {
    convert(ce, "Generation failed", x.getParent());
  }
}

// **************************************************************************
// *                                                                        *
// *                       meddly_otfimplsat  class                         *
// *                                                                        *
// **************************************************************************

/** On-the-fly-implicit saturation using Meddly.
 
 REFERRED AS IMPLICIT-RELATION IN Meddly
 HACK!
 RE-DESIGN THIS CLASS AND EVERYTHING ELSE IN HERE!
 IT'S ALL CRAP!
 
 */

class meddly_otfimplsat : public meddly_implicitgen {
public:
  meddly_otfimplsat();
  virtual ~meddly_otfimplsat();
protected:
  virtual void buildRSS(meddly_varoption &x);
  virtual void generateRSS(meddly_varoption &x, timer &w) {
    DCASSERT(0);
  }
  virtual const char* getAlgName() const { return "on the fly implicit nodes saturation"; }
private:
  // Build the otfimpl next-state function
  //
  MEDDLY::satimpl_opname::implicit_relation* buildNSF(meddly_varoption &x);

  // Build the reachable set of states
  //
  void generateRSS(meddly_varoption &x, MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF);
  // Clear Meddly's compute tables
  //
  void clearMeddlyComputeTable(meddly_varoption &x,MEDDLY::satimpl_opname::implicit_relation &IMPL_NSF);
  // Compute the maximum number of tokens that can occur at any place in the Petri Net
  //
  long computeMaxTokensInPlace(meddly_varoption &x,MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF);
  
  // Compute the number of arcs in the reachability graph
  //
  bigint computeNumTransitions(const MEDDLY::dd_edge &RS,
                               MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF);
  bigint computeNumTransitions(
                               MEDDLY::node_handle mdd, MEDDLY::node_handle mxd, int level,
                               MEDDLY::expert_forest* mddf, MEDDLY::expert_forest* mxdf,
                               NodeNodeIntMap &ct);
  
  // Compute the maximum number of tokens in any state of the reachable set of states
  //
 bool computeMaxTokensPerLevel(
                                meddly_varoption &x,
                                MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF,
                                std::vector<long> &result,
                                std::vector< std::vector<long> > &level_index_to_token);
  long computeMaxTokensPerMarking(
                                  const std::vector<long> &level_to_max_tokens,
                                  const std::vector< std::vector<long> > &level_index_to_token,
                                  MEDDLY::expert_forest* mddf,
                                  MEDDLY::node_handle mdd,
                                  int level,
                                  std::unordered_map < MEDDLY::node_handle, long > &ct);
  long computeMaxTokensPerMarking(
                                  meddly_varoption &x,
                                  MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF);
  
  // Compute the number of arcs in the reachability graph using implicit relations
  //
  bigint computeNumTransitionsImplRel(const MEDDLY::dd_edge &RS,
                               MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF);
  bigint computeNumTransitionsImplRel(
                               MEDDLY::node_handle mdd, MEDDLY::node_handle mxd, int level,
                               MEDDLY::expert_forest* mddf, MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF,
                               NodeNodeIntMap &ct);
  
};

meddly_otfimplsat the_meddly_otfimplsat;

meddly_otfimplsat::meddly_otfimplsat() : meddly_implicitgen()
{
}

meddly_otfimplsat::~meddly_otfimplsat()
{
}

void meddly_otfimplsat::buildRSS(meddly_varoption &x)
{
  timer watch;
  timer subwatch;
  if (startGen(x.getParent())) {
    Report().stopIO();
  }
  
  //
  // Build the initial state set, and other initializations
  //
  if (Report().startReport()) {
    Report().report() << "Initializing forests\n";
    Report().stopIO();
  }
  
  try {
    x.initializeVars();
    
    if (Report().startReport()) {
      Report().report() << "Initialized  forests, took ";
      Report().report() << watch.elapsed_seconds() << " seconds\n";
      Report().stopIO();
    }
    
    x.initializeEvents(Debug());
    
    //
    // Build next-state function
    //
    if (Report().startReport()) {
      Report().report() << "Initializing next-state function builder\n";
      subwatch.reset();
      Report().stopIO();
    }
    
    MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF = buildNSF(x);
    DCASSERT(IMPL_NSF);
    
    
    if (Report().startReport()) {
      Report().report() << "Initialized  next-state function builder, took ";
      Report().report() << subwatch.elapsed_seconds() << " seconds\n";
      Report().stopIO();
    }
    //
    // Generate reachability set
    //
    if (Report().startReport()) {
      Report().report() << "Building reachability set\n";
      Report().stopIO();
      subwatch.reset();
    }
    
    generateRSS(x, IMPL_NSF);
    
    if (Report().startReport()) {
      Report().report() << "Built reachability set, took ";
      Report().report() << subwatch.elapsed_seconds() << " seconds\n";
      smart_output Drep(Report().report());
      MEDDLY::operation::showAllComputeTables(Drep,3);
      Report().stopIO();
    }
    
    if (stopGen(false, x.getParent(), watch)) {
      reportGen(false, Report().report());
      x.reportStats(Report().report());
      // Report().report() << "\tMinterms:\t" << NSF->mintermMemoryUsage() << "  bytes\n";
      Report().stopIO();
    }

    result numstates;
    x.getNumStates(numstates);
    if (!numstates.isNormal()) {
      //
      // TBD: Error, can we print something and exit cleanly here?
      em->cout() << "CANNOT COMPUTE\n";
      delete IMPL_NSF;
      return;
    }
    
    em->cout() << "STATE_SPACE STATES ";
    shared_object* bigns = numstates.getPtr();
    if (bigns) {
      bigns->Print(em->cout(), 0);
    } else {
      long ns = numstates.getInt();
      em->cout() << ns;
    }
    em->cout() << " TECHNIQUES SEQUENTIAL_PROCESSING DECISION_DIAGRAMS\n";
    em->cout().flush();
    
    // Build a monolithic transition relation
    MEDDLY::dd_edge NSF = IMPL_NSF->buildMxdForest();
    
    
    
#if 1
    //
    // Compute maximum tokens in any place
    //
    long max_tokens_in_place = computeMaxTokensInPlace(x, IMPL_NSF);
    em->cout() << "STATE_SPACE MAX_TOKEN_IN_PLACE " << max_tokens_in_place;
    em->cout() << " TECHNIQUES SEQUENTIAL_PROCESSING DECISION_DIAGRAMS\n";
    em->cout().flush();

    //
    // Compute the maximum number of tokens in a marking
    //
    long max_tokens_per_marking = computeMaxTokensPerMarking(x, IMPL_NSF);
    em->cout() << "STATE_SPACE MAX_TOKEN_PER_MARKING " << max_tokens_per_marking;
    em->cout() << " TECHNIQUES SEQUENTIAL_PROCESSING DECISION_DIAGRAMS\n";
    em->cout().flush();

#if 0
    //
    // Compute number of arcs using mxd
    //
    bigint num_transitions_rel = computeNumTransitions(x.getStates(), IMPL_NSF);
    em->cout() << "STATE_SPACE TRANSITIONS ";
    num_transitions_rel.Print(em->cout(), 0);
    em->cout() << " TECHNIQUES SEQUENTIAL_PROCESSING DECISION_DIAGRAMS\n";
    em->cout().flush();
#endif
    
    //
    // Compute number of arcs using implicit relations
    //
    bigint num_transitions = computeNumTransitionsImplRel(x.getStates(), IMPL_NSF);
    em->cout() << "STATE_SPACE TRANSITIONS ";
    num_transitions.Print(em->cout(), 0);
    em->cout() << " TECHNIQUES SEQUENTIAL_PROCESSING DECISION_DIAGRAMS\n";
    em->cout().flush();
#endif
    
    
    
    //delete IMPL_NSF;
    
  } // try
  
  catch (subengine::error status) {
    if (stopGen(true, x.getParent(), watch)) Report().stopIO();
    throw status;
  }
}

void meddly_otfimplsat::clearMeddlyComputeTable(meddly_varoption &x,MEDDLY::satimpl_opname::implicit_relation &IMPL_NSF)
{
  //NSF.clearMinterms();
  MEDDLY::operation::removeAllFromMonolithic();
}

MEDDLY::satimpl_opname::implicit_relation*
meddly_otfimplsat::buildNSF(meddly_varoption &x)
{
  // FOR NOW!
  // TBD!
  return x.buildNSF_IMPLICIT(Debug());
}

void meddly_otfimplsat::generateRSS(meddly_varoption &x,
                                    MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF)
{
  using namespace MEDDLY;
  
  DCASSERT(IMPL_NSF);
  DCASSERT(MEDDLY::SATURATION_IMPL_FORWARD);
  try{
    specialized_operation* satop = SATURATION_IMPL_FORWARD->buildOperation(IMPL_NSF);
    DCASSERT(satop);
    
    shared_ddedge* S = x.newMddEdge();
    satop->compute(x.getInitial(), S->E);
    x.setStates(S);
    checkTerm("Generation failed", x.getParent());
  }
  catch (MEDDLY::error ce) {
    convert(ce, "Generation failed", x.getParent());
  }
}

//
//Place Token Count
//
long meddly_otfimplsat::computeMaxTokensInPlace(
                                            meddly_varoption &x, MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF)
{
  substate_colls* colls = x.getSubstateStorage();
  if (0 == colls) return -1;
  
  const dsde_hlm& p = x.getParent();
  shared_state* foo = new shared_state(&p);
  long max_tokens = 0;
  for (int k = 1; k <= p.getPartInfo().num_levels; k++) {
    long nConfirmed = IMPL_NSF->getConfirmedStates(k);
    int sz = foo->readSubstateSize(k);
    int chunk[sz];
    int curr = 0;
    for (int i = 0; i < nConfirmed; i++, curr++) {
      while (!IMPL_NSF->isConfirmedState(k, curr)) curr++;
      // std::cout << "k: " << k << ", i: " << i << ", curr: " << curr << ", sz:" << sz << "\n";
      colls->getSubstate(k, curr, chunk, sz);
      for (int j = 0; j < sz; j++) {
        // std::cout << "\t\t chunk[" << j << "]: " << chunk[j] << "\n" <<"maxtokens="<<max_tokens;
        if (max_tokens < chunk[j]) max_tokens = chunk[j];
      }
    }
  }
  return max_tokens;
}

//
//Marking Token Count
//

bool meddly_otfimplsat::computeMaxTokensPerLevel(
                                             meddly_varoption &x,
                                             MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF,
                                             std::vector<long> &max_tokens,
                                             std::vector< std::vector<long> > &level_index_to_token)
{
  substate_colls* colls = x.getSubstateStorage();
  if (0 == colls) return false;
  
  const dsde_hlm& p = x.getParent();
  shared_state* foo = new shared_state(&p);
  
  max_tokens.resize(p.getPartInfo().num_levels+1);
  for (std::vector<long>::iterator rIter = max_tokens.begin();
       rIter != max_tokens.end(); rIter++) {
    *rIter = 0;
  }
  
  level_index_to_token.resize(p.getPartInfo().num_levels+1);
  for (int i = 0; i < int(level_index_to_token.size()); i++) {
    for (std::vector<long>::iterator rIter = level_index_to_token[i].begin();
         rIter != level_index_to_token[i].end(); rIter++) {
      *rIter = 0;
    }
  }
  
  for (int k = 1; k <= p.getPartInfo().num_levels; k++) {
    long nConfirmed = IMPL_NSF->getConfirmedStates(k);
    int sz = foo->readSubstateSize(k);
    int chunk[sz];
    int curr = 0;
    for (int i = 0; i < nConfirmed; i++, curr++) {
      //std::cout << "k: " << k << ", i: " << i << ", curr: " << curr << ", sz:" << sz << "\n";
      while (!IMPL_NSF->isConfirmedState(k, curr)) curr++;
      level_index_to_token[k].resize(curr+1, 0);
      colls->getSubstate(k, curr, chunk, sz);
      int sum = 0;
      for (int j = 0; j < sz; j++) {
        //std::cout << "\t\t chunk[" << j << "]: " << chunk[j] << "\n";
        sum += chunk[j];
      }
      level_index_to_token[k][curr] = sum;
      if (max_tokens[k] < sum) max_tokens[k] = sum;
      //std::cout << "\t\t max: " << max_tokens[k] << "\n";
    }
    //if(nConfirmed==0) {level_index_to_token[k].resize(1, 0); level_index_to_token[k][0] = 0; max_tokens[k] =0;}
  }
  return true;
}


long meddly_otfimplsat::computeMaxTokensPerMarking(
                                               const std::vector<long> &level_to_max_tokens,
                                               const std::vector< std::vector<long> > &level_index_to_token,
                                               MEDDLY::expert_forest* mddf,
                                               MEDDLY::node_handle mdd,
                                               int level, 
                                               std::unordered_map < MEDDLY::node_handle, long > &ct)
{
  DCASSERT(0 != mdd);
  if (0 == level) return 0;
  
   
  int mddLevel = mddf->getNodeLevel(mdd);
  long result = 0;
  
  if (mddLevel < level) {
    // if mddLevel < level
    // --- level was fully skipped, return level_size * compute(mdd, mxd, level-1)
    result = level_to_max_tokens[level] +
    computeMaxTokensPerMarking(level_to_max_tokens, level_index_to_token,
                               mddf, mdd, level-1, ct);
  } else {
    // check compute table
    std::unordered_map < MEDDLY::node_handle, long >::iterator iter = ct.find(mdd);
    if (iter != ct.end()) {
      return iter->second;
    }
    // expand mdd
    MEDDLY::unpacked_node* mdd_nr = MEDDLY::unpacked_node::newFromNode(mddf, mdd, false);
    for (int i = 0; i < mdd_nr->getNNZs(); i++) {
    result = MAX( result,
                   (level_index_to_token[level][mdd_nr->i(i)] +
                    computeMaxTokensPerMarking(level_to_max_tokens, level_index_to_token,
                                               mddf, mdd_nr->d(i), level-1, ct)) );
    }
    MEDDLY::unpacked_node::recycle(mdd_nr);
    // insert into compute table
    ct[mdd] = result;
  }
  
  return result;
}

long meddly_otfimplsat::computeMaxTokensPerMarking(
                                               meddly_varoption &x,
                                                MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF)
{
  // traverse the set of reachable states
  //
  // for all children node,
  //    max = MAX(max, map_i_to_token(level, i) + max_marking(child[i], level-1))
  
  const MEDDLY::dd_edge& states = x.getStates();
  if (0 == states.getNode()) return 0;
  
  std::vector<long> level_to_max_tokens;
  std::vector< std::vector<long> > level_index_to_token;
  if (!computeMaxTokensPerLevel(x, IMPL_NSF, level_to_max_tokens, level_index_to_token))
    return -1;
  //for (int i = 0; i < int(level_to_max_tokens.size()); i++) {
  //  em->cout() << "Level: " << i << ", max tokens: " << level_to_max_tokens[i] << "\n";
  //}
  
  std::unordered_map < MEDDLY::node_handle, long > ct;
  return computeMaxTokensPerMarking(
                                    level_to_max_tokens,
                                    level_index_to_token,
                                    smart_cast<MEDDLY::expert_forest*>(states.getForest()),
                                    states.getNode(),
                                    x.getParent().getPartInfo().num_levels,
                                    ct);
}


//
//Transition Count
//

bigint meddly_otfimplsat::computeNumTransitions(
                                            MEDDLY::node_handle mdd, MEDDLY::node_handle mxd, int level,
                                            MEDDLY::expert_forest* mddf, MEDDLY::expert_forest* mxdf,
                                            NodeNodeIntMap &ct)
{
  if (0 == mdd || 0 == mxd) return bigint(0l);
  if (0 == level) return bigint(1l);
  
  int mddLevel = mddf->getNodeLevel(mdd);
  int mxdLevel = mxdf->getNodeLevel(mxd);
  int levelSize = mddf->getLevelSize(level);
  bigint result;
  
  DCASSERT(ABS(mxdLevel) <= level);
  
  if (mddLevel < level) {
    if (ABS(mxdLevel) < level) {
      // if mddLevel < level && ABS(mxdLevel) < level
      // --- level was fully skipped, return level_size * compute(mdd, mxd, level-1)
      result.mul(bigint(levelSize), computeNumTransitions(mdd, mxd, level-1, mddf, mxdf, ct));
    } else if (mxdLevel < 0) {
      // if mddLevel < level && ABS(mxdLevel) >= level && mxdLevel < 0
      // --- return level_size * compute(mdd, mxd[i], level-1)
      MEDDLY::unpacked_node* p_nr = MEDDLY::unpacked_node::newFromNode(mxdf, mxd, false);
      for (int i = 0; i < p_nr->getNNZs(); i++) {
        result.add(result, computeNumTransitions(mdd, p_nr->d(i), level-1, mddf, mxdf, ct));
      }
      MEDDLY::unpacked_node::recycle(p_nr);
      result.mul(bigint(levelSize), result);
    } else {
      // expand mxd
      MEDDLY::unpacked_node* up_nr = MEDDLY::unpacked_node::newFromNode(mxdf, mxd, false);
      MEDDLY::unpacked_node* p_nr = MEDDLY::unpacked_node::useUnpackedNode();
      for (int i = 0; i < up_nr->getNNZs(); i++) {
        p_nr->initFromNode(mxdf, up_nr->d(i), false);
        for (int j = 0; j < p_nr->getNNZs(); j++) {
          result.add(result, computeNumTransitions(mdd, p_nr->d(j), level-1, mddf, mxdf, ct));
        }
      }
      MEDDLY::unpacked_node::recycle(p_nr);
      MEDDLY::unpacked_node::recycle(up_nr);
    }
  } else {
    DCASSERT(mddLevel == level);
    // check compute table
    NodeNodeIntMap::iterator iter1 = ct.find(mdd);
    NodeIntMap::iterator iter2;
    if (iter1 != ct.end()) {
      iter2 = iter1->second.find(mxd);
      if (iter2 != iter1->second.end()) {
        // found
        return iter2->second;
      }
    }
    
    // expand mdd
    MEDDLY::unpacked_node* mdd_nr = MEDDLY::unpacked_node::newFromNode(mddf, mdd, false);
    if (ABS(mxdLevel) < level) {
      for (int i = 0; i < mdd_nr->getNNZs(); i++) {
        result.add(result, computeNumTransitions(mdd_nr->d(i), mxd, level-1, mddf, mxdf, ct));
      }
    } else if (mxdLevel < 0) {
      MEDDLY::unpacked_node* p_nr = MEDDLY::unpacked_node::newFromNode(mxdf, mxd, false);
      for (int i = 0; i < mdd_nr->getNNZs(); i++) {
        for (int j = 0; j < p_nr->getNNZs(); j++) {
          result.add(result, computeNumTransitions(mdd_nr->d(i), p_nr->d(j), level-1, mddf, mxdf, ct));
        }
      }
      MEDDLY::unpacked_node::recycle(p_nr);
    } else {
      // expand mxd
      MEDDLY::unpacked_node* up_nr = MEDDLY::unpacked_node::newFromNode(mxdf, mxd, true);
      MEDDLY::unpacked_node* p_nr = MEDDLY::unpacked_node::useUnpackedNode();
      for (int i = 0; i < mdd_nr->getNNZs(); i++) {
        int i_index = mdd_nr->i(i);
        if (0 == up_nr->d(i_index)) continue;
        if (-1 == up_nr->d(i_index)) {
          result.add(result, computeNumTransitions(mdd_nr->d(i), -1, level-1, mddf, mxdf, ct));
        } else {
          p_nr->initFromNode(mxdf, up_nr->d(i_index), false);
          for (int j = 0; j < p_nr->getNNZs(); j++) {
            result.add(result, computeNumTransitions(mdd_nr->d(i), p_nr->d(j), level-1, mddf, mxdf, ct));
          }
        }
      }
      MEDDLY::unpacked_node::recycle(p_nr);
      MEDDLY::unpacked_node::recycle(up_nr);
    }
    MEDDLY::unpacked_node::recycle(mdd_nr);
    
    // insert into compute table
    ct[mdd].insert(std::pair<MEDDLY::node_handle, bigint>(mxd, result));
  }
  
  return result;
}


bigint meddly_otfimplsat::computeNumTransitions(
                                            const MEDDLY::dd_edge &RS,
                                                MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF)
{
  // num_trans = 0
  // for each event e in NSF,
  //    num_trans += computeNumTransitions(rs, e)
  
  int num_vars = RS.getForest()->getDomain()->getNumVariables();
  MEDDLY::node_handle mdd = RS.getNode();
  
  bigint num_transitions = 0l;
  for (int i = 1; i <= num_vars; i++)
    {
  for (int ei = 0; ei < IMPL_NSF->lengthForLevel(i); ei++) 
    {
  NodeNodeIntMap ct;
  MEDDLY::domain *d = IMPL_NSF->getOutForest()->useDomain();
  MEDDLY::forest* mxd = d->createForest(true,MEDDLY::forest::BOOLEAN, MEDDLY::forest::MULTI_TERMINAL);
  MEDDLY::forest* event_mxd = d->createForest(true, MEDDLY::forest::INTEGER, MEDDLY::forest::MULTI_TERMINAL);
 
  MEDDLY::dd_edge nsf_ev(mxd);
  nsf_ev = IMPL_NSF->buildEventMxd(IMPL_NSF->arrayForLevel(i)[ei],mxd);
  MEDDLY::node_handle handle_mxd = nsf_ev.getNode();//IMPL_NSF->buildMxdForest().getNode();
  bigint temp = computeNumTransitions(mdd, handle_mxd, num_vars,
                                      IMPL_NSF->getInForest(), dynamic_cast<MEDDLY::expert_forest*>(nsf_ev.getForest()), ct);
  num_transitions.add(num_transitions, temp);
    }
  }
  return num_transitions;
}


//
//Transition Count Implicit Relations
//

bigint meddly_otfimplsat::computeNumTransitionsImplRel(
                                                MEDDLY::node_handle mdd, MEDDLY::node_handle mxd, int level,
                                                MEDDLY::expert_forest* mddf, MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF,
                                                NodeNodeIntMap &ct)
{
  if (0 == mdd || 0 == mxd) return bigint(0l);
  if (0 == level) return bigint(1l);
  
  int mddLevel = mddf->getNodeLevel(mdd);
  MEDDLY::satimpl_opname::relation_node* mxdNode = IMPL_NSF->nodeExists(mxd);
  int mxdLevel = mxdNode->getLevel();
  int levelSize = mddf->getLevelSize(level);
  bigint result;
  
  DCASSERT(mxdLevel <= level);
  
  if (mddLevel < level) {
    if (mxdLevel < level) {
      // if mddLevel < level && mxdLevel < level
      // --- level was fully skipped, return level_size * compute(mdd, mxd, level-1)
      result.mul(bigint(levelSize), computeNumTransitionsImplRel(mdd, mxd, level-1, mddf, IMPL_NSF, ct));
    } else {
      // expand mxd
      MEDDLY::node_handle downMxd = mxdNode->getDown();
      
      for (int i = 0; i < mxdNode->getPieceSize(); i++) {
        if(mxdNode->getTokenUpdate()[i]>=0)
          result.add(result, computeNumTransitionsImplRel(mdd, downMxd, level-1, mddf, IMPL_NSF, ct));
        }
      }
  } else {
    DCASSERT(mddLevel == level);
    // check compute table
    NodeNodeIntMap::iterator iter1 = ct.find(mdd);
    NodeIntMap::iterator iter2;
    if (iter1 != ct.end()) {
      iter2 = iter1->second.find(mxd);
      if (iter2 != iter1->second.end()) {
        // found
        return iter2->second;
      }
    }
    
    // expand mdd
    MEDDLY::unpacked_node* mdd_nr = MEDDLY::unpacked_node::newFromNode(mddf, mdd, false);
    if (mxdLevel < level) {
      for (int i = 0; i < mdd_nr->getNNZs(); i++) {
        result.add(result, computeNumTransitionsImplRel(mdd_nr->d(i), mxd, level-1, mddf, IMPL_NSF, ct));
      }
    } else {
      // expand mxd
      for (int i = 0; i < mdd_nr->getNNZs(); i++) {
        int i_index = mdd_nr->i(i);
        if (mxdNode->getTokenUpdate()[i_index]<0) continue;
        MEDDLY::node_handle downMxd = mxdNode->getDown();
        if(mxdNode->getTokenUpdate()[i_index]>=0)
          result.add(result, computeNumTransitionsImplRel(mdd_nr->d(i), downMxd, level-1, mddf, IMPL_NSF, ct));
        }
     }
    MEDDLY::unpacked_node::recycle(mdd_nr);
    
    // insert into compute table
    ct[mdd].insert(std::pair<MEDDLY::node_handle, bigint>(mxd, result));
  }
  
  return result;
}


bigint meddly_otfimplsat::computeNumTransitionsImplRel(
                                                const MEDDLY::dd_edge &RS,
                                                MEDDLY::satimpl_opname::implicit_relation* IMPL_NSF)
{
  // num_trans = 0
  // for each event e in NSF,
  //    num_trans += computeNumTransitions(rs, e)
  
  int num_vars = RS.getForest()->getDomain()->getNumVariables();
  MEDDLY::node_handle mdd = RS.getNode();
  
  bigint num_transitions = 0l;
  for (int i = 1; i <= num_vars; i++)
    {
#ifdef DEBUG_NUM_TRANS
    em->cout() << "Level : " << i << "\t#Events: " << IMPL_NSF->lengthForLevel(i) << "\n";
#endif
    for (int ei = 0; ei < IMPL_NSF->lengthForLevel(i); ei++) 
      {
      NodeNodeIntMap ct;
      bigint temp = computeNumTransitionsImplRel(mdd, IMPL_NSF->arrayForLevel(i)[ei], num_vars,
                                          IMPL_NSF->getOutForest(), IMPL_NSF, ct);

      num_transitions.add(num_transitions, temp);
      }
    }
  return num_transitions;
}

// **************************************************************************
// *                                                                        *
// *                        meddly_traditional class                        *
// *                                                                        *
// **************************************************************************

/** Traditional implicit generation as implemented in Meddly.
*/
class meddly_traditional : public meddly_implicitgen {
public:
  meddly_traditional();
  virtual ~meddly_traditional();
protected:
  virtual void generateRSS(meddly_varoption &x, timer &w);
  virtual const char* getAlgName() const { return "traditional"; }
};

meddly_traditional the_meddly_traditional;

// **************************************************************************
// *                       meddly_traditional methods                       *
// **************************************************************************

meddly_traditional::meddly_traditional() : meddly_implicitgen()
{
}

meddly_traditional::~meddly_traditional()
{
}

void meddly_traditional::generateRSS(meddly_varoption &x, timer&)
{
  try {
    shared_ddedge* S = x.newMddEdge();
    MEDDLY::apply(
      MEDDLY::REACHABLE_STATES_BFS,
      x.getInitial(), 
      getNSF(),
      S->E
    );
    x.setStates(S);
    return checkTerm("Generation failed", x.getParent());
  }
  catch (MEDDLY::error ce) {
    return convert(ce, "Generation failed", x.getParent());
  }
}


// **************************************************************************
// *                                                                        *
// *                         meddly_iterative class                         *
// *                                                                        *
// **************************************************************************

/** Abstract base class for iterative generation algorithms.
*/
class meddly_iterative : public meddly_implicitgen {
protected:
  long iterations;
public:
  meddly_iterative();
  virtual ~meddly_iterative();
protected:
  virtual void reportGen(bool err, DisplayStream &s) const;
  virtual void initGen();
  virtual void doneGen();
};

// **************************************************************************
// *                        meddly_iterative methods                        *
// **************************************************************************

meddly_iterative::meddly_iterative() : meddly_implicitgen()
{
}

meddly_iterative::~meddly_iterative()
{
}

void meddly_iterative::reportGen(bool err, DisplayStream &s) const
{
  if (0==iterations) return;
  s << "\t" << iterations;
  if (err) s << " iterations until error\n";
  else     s << " iterations required\n";
}

void meddly_iterative::initGen() 
{ 
  iterations = 0;
  em->waitTerm(); 
}

void meddly_iterative::doneGen() 
{ 
  em->resumeTerm(); 
}

// **************************************************************************
// *                                                                        *
// *                         meddly_frontier  class                         *
// *                                                                        *
// **************************************************************************

/** Traditional implicit generation using frontier sets.
*/
class meddly_frontier : public meddly_iterative {
public:
  meddly_frontier();
  virtual ~meddly_frontier();
protected:
  virtual void generateRSS(meddly_varoption &x, timer &w);
  virtual const char* getAlgName() const { return "frontier"; }
};

meddly_frontier the_meddly_frontier;

// **************************************************************************
// *                        meddly_frontier  methods                        *
// **************************************************************************

meddly_frontier::meddly_frontier() : meddly_iterative()
{
}

meddly_frontier::~meddly_frontier()
{
}

void meddly_frontier::generateRSS(meddly_varoption &x, timer &w)
{
  MEDDLY::dd_edge F = x.getInitial();
  shared_ddedge* S = x.newMddEdge();
  S->E = x.getInitial();
  while (F.getNode()) {
    iterations++;
    if (Debug().startReport()) {
      Debug().report() << "Starting iteration ";
      Debug().report().Put(iterations, 5);
      Debug().report() << ":\n";
      Debug().stopIO();
    }
    // compute N(F)
    try {
      MEDDLY::apply(MEDDLY::POST_IMAGE, F, getNSF(), F);
      checkTerm("post-image", x.getParent());
    }
    catch (MEDDLY::error ce) {
      convert(ce, "post-image", x.getParent());
    }
    if (Debug().startReport()) {
      Debug().report() << "\tdone F:=N(F)\n";
      Debug().stopIO();
    }
    // subtract S
    try {
      MEDDLY::apply(MEDDLY::DIFFERENCE, F, S->E, F);
      checkTerm("set difference", x.getParent());
    }
    catch (MEDDLY::error ce) {
      convert(ce, "set difference", x.getParent());
    }
    if (Debug().startReport()) {
      Debug().report() << "\tdone F:=F-S  ";
      double card = F.getCardinality();
      Debug().report().Put(card, 13);
      Debug().report() << " states in frontier set\n";
      Debug().stopIO();
    }
    // add F to S
    try {
      MEDDLY::apply(
        MEDDLY::UNION, S->E, F, S->E
      );
      checkTerm("set union", x.getParent());
    }
    catch (MEDDLY::error ce) {
      convert(ce, "set union", x.getParent());
    }
    if (Debug().startReport()) {
      Debug().report() << "\tdone S:=S+F  ";
      double card = S->E.getCardinality();
      Debug().report().Put(card, 13);
      Debug().report() << " reachable states so far";
      Debug().newLine();
      long nodes = x.getMddForest()->getCurrentNumNodes();
      Debug().report() << nodes << " nodes in forest, ";
      Debug().report() << w.elapsed_seconds() << " seconds total time\n";
      Debug().stopIO();
    }
  } // while F
  x.setStates(S);
}

// **************************************************************************
// *                                                                        *
// *                          meddly_nextall class                          *
// *                                                                        *
// **************************************************************************

/** Iterative implicit generation.
    We do a post-image of all reachable states at each iteration.
*/
class meddly_nextall : public meddly_iterative {
public:
  meddly_nextall();
  virtual ~meddly_nextall();
protected:
  virtual void generateRSS(meddly_varoption &x, timer &w);
  virtual const char* getAlgName() const { return "next-all"; }
};

meddly_nextall the_meddly_nextall;

// **************************************************************************
// *                         meddly_nextall methods                         *
// **************************************************************************

meddly_nextall::meddly_nextall() : meddly_iterative()
{
}

meddly_nextall::~meddly_nextall()
{
}

void meddly_nextall::generateRSS(meddly_varoption &x, timer &w)
{
  MEDDLY::dd_edge Old(x.getMddForest());
  x.getMddForest()->createEdge(false, Old);
  shared_ddedge* S = x.newMddEdge();
  S->E = x.getInitial();
  while (S->E != Old) {
    iterations++;
    if (Debug().startReport()) {
      Debug().report() << "Starting iteration ";
      Debug().report().Put(iterations, 5);
      Debug().report() << ":\n";
      Debug().stopIO();
    }
    Old = S->E;
    // compute S = N(S)
    try {
      MEDDLY::apply(MEDDLY::POST_IMAGE, 
                    S->E, getNSF(), S->E);
      checkTerm("post-image", x.getParent());
    }
    catch (MEDDLY::error ce) {
      convert(ce, "post-image", x.getParent());
    }
    if (Debug().startReport()) {
      Debug().report() << "\tdone S':=N(S)\n";
      Debug().stopIO();
    }
    // compute S = Old + S
    try {
      MEDDLY::apply(MEDDLY::UNION, 
                  S->E, Old, S->E);
      checkTerm("set union", x.getParent());
    }
    catch (MEDDLY::error ce) {
      convert(ce, "set union", x.getParent());
    }
    if (Debug().startReport()) {
      Debug().report() << "\tdone S:=S+S'  ";
      double card = S->E.getCardinality();
      Debug().report().Put(card, 13);
      Debug().report() << " reachable states so far";
      Debug().newLine();
      long nodes = x.getMddForest()->getCurrentNumNodes();
      Debug().report() << nodes << " nodes in forest, ";
      Debug().report() << w.elapsed_seconds() << " seconds total time\n";
      Debug().stopIO();
    }
  } // while F
  x.setStates(S);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_saturmeddly : public initializer {
  public:
    init_saturmeddly();
    virtual bool execute();
};
init_saturmeddly the_saturmeddly_initializer;

init_saturmeddly::init_saturmeddly() : initializer("init_saturmeddly")
{
  usesResource("em");
  usesResource("meddlyprocgen");
}

bool init_saturmeddly::execute()
{
  if (0==em) return false;

  RegisterEngine(em,
    "MeddlyProcessGeneration",
    "SATURATION",
    "The Saturation algorithm, as implemented in Meddly",
    &the_meddly_saturation
  );

  RegisterEngine(em,
    "MeddlyProcessGeneration",
    "OTF_SATURATION",
    "The On-the-fly Saturation algorithm, as implemented in Meddly",
    &the_meddly_otfsat
  );

  RegisterEngine(em,
   "MeddlyProcessGeneration",
   "OTF_IMPLICIT_SATURATION",
   "The On-the-fly-implicit Saturation algorithm, as implemented in Meddly",
    &the_meddly_otfimplsat
   );
  

  RegisterEngine(em,
    "MeddlyProcessGeneration",
    "TRADITIONAL",
    "A traditional iterative algorithm, as implemented in Meddly",
    &the_meddly_traditional
  );

  RegisterEngine(em,
    "MeddlyProcessGeneration",
    "FRONTIER",
    "A traditional iterative algorithm using frontier set construction",
    &the_meddly_frontier
  );

  RegisterEngine(em,
    "MeddlyProcessGeneration",
    "NEXT_ALL",
    "An iterative algorithm that unions the post-image of all reachable states",
    &the_meddly_nextall
  );

  // Options

  radio_button** orders = new radio_button*[3];
  orders[0] = new radio_button(
    "HIGH_TO_LOW", 
    "Events whose group dependencies are higher are processed first",
    meddly_implicitgen::ORDER_HIGH_TO_LOW
  );
  orders[1] = new radio_button(
    "LOW_TO_HIGH", 
    "Events whose group dependencies are lower are processed first",
    meddly_implicitgen::ORDER_LOW_TO_HIGH
  );
  orders[2] = new radio_button(
    "MODEL", 
    "Events are processed in order of declaration in the model",
    meddly_implicitgen::ORDER_MODEL
  );
  meddly_implicitgen::order_policy = meddly_implicitgen::ORDER_LOW_TO_HIGH;
  em->addOption(
    MakeRadioOption(
      "MeddlyEventOrder",
      "Order to add events to the next state function, for implicit generation algorithms using Meddly.",
      orders, 3, meddly_implicitgen::order_policy
    )
  );

  return true;
}

#endif // HAVE_MEDDLY_H
