
// $Id$

#include "satur_meddly.h"

#define PROC_MEDDLY_DETAILS
#include "proc_meddly.h"

#include "../Options/options.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/engine.h"

#define DSDE_HLM_DETAILS
#include "../Formlsms/dsde_hlm.h"
#include "../Formlsms/check_llm.h"
#include "../Formlsms/rss_mdd.h"
#include "../Formlsms/fsm_mdd.h"
#include "../Formlsms/mc_llm.h"
#include "../Formlsms/mc_mdd.h"

#include "../Modules/glue_meddly.h"

#include "timerlib.h"

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
public:
  mxd_fsm_finish(bool pot, meddly_varoption* mvo);
  virtual ~mxd_fsm_finish();
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual void RunEngine(hldsm* m, result &statesonly);
};

mxd_fsm_finish::mxd_fsm_finish(bool pot, meddly_varoption* _mvo) 
: process_generator()
{
  potential = pot;
  mvo = _mvo;
}

mxd_fsm_finish::~mxd_fsm_finish()
{
  delete mvo;
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

  meddly_states* rss = GrabMeddlyFSMStates(lm);
  DCASSERT(rss);

  timer watch;
  if (report.startReport()) {
    em->report() << "Finishing FSM using Meddly\n";
    em->report() << "\tUsing ";
    em->report() << ( potential ? "potential edges\n" : "actual edges\n" );
    em->stopIO();
  }

  rss->proc_wrap = Share(rss->mxd_wrap);
  
  FinishMeddlyFSM(lm, potential);

  if (report.startReport()) {
    em->report() << "Finished  FSM using Meddly, took ";
    em->report() << watch.elapsed_seconds() << " seconds\n";
    em->stopIO();
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
public:
  mxd_mc_finish(bool pot, meddly_varoption* mvo);
  virtual ~mxd_mc_finish();
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual void RunEngine(hldsm* m, result &statesonly);
};

mxd_mc_finish::mxd_mc_finish(bool pot, meddly_varoption* _mvo)
: process_generator()
{
  potential = pot;
  mvo = _mvo;
}

mxd_mc_finish::~mxd_mc_finish()
{
  delete mvo;
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

  timer watch;
  if (report.startReport()) {
    em->report() << "Finishing CTMC using Meddly\n";
    em->report() << "\tUsing ";
    em->report() << ( potential ? "potential edges\n" : "actual edges\n" );
    em->stopIO();
  }

  //
  // Build new forest for CTMC
  //
  meddly_states* rss = GrabMeddlyMCStates(lm);
  DCASSERT(rss);
  // TBD: mtmdd versus ev*mdd?
  MEDDLY::forest* newf = rss->createForest(
    true, MEDDLY::forest::REAL, MEDDLY::forest::MULTI_TERMINAL
  );
  rss->proc_wrap = 
    rss->mxd_wrap->copyWithDifferentForest("MC MTMxD", newf);

  //
  // Build initial CTMC
  //
  DCASSERT(mvo);
  const dsde_hlm &m = mvo->parent;

  shared_ddedge* R = smart_cast<shared_ddedge*>(rss->proc_wrap->makeEdge(0));
  DCASSERT(R);
  rss->proc_wrap->buildSymbolicConst(0.0, R);

  //
  //  Add rates for each event to CTMC.
  //  
  traverse_data x(traverse_data::BuildDD);
  result ans;
  x.answer = &ans;
  MEDDLY::dd_edge tmp(newf);
  for (long e=0; e<m.getNumEvents(); e++) {
    // Get rate DD for this event
    x.ddlib = rss->proc_wrap;
    x.which = traverse_data::BuildExpoRateDD;
    DCASSERT(m.readEvent(e));
    expr* distro = m.readEvent(e)->getDistribution();
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


  FinishMeddlyFSM(lm, potential);
  Delete(R);

  if (report.startReport()) {
    em->report() << "Finished  CTMC using Meddly, took ";
    em->report() << watch.elapsed_seconds() << " seconds\n";
    rss->proc_wrap->reportStats(em->report());
#ifdef DEBUG_FINAL_CTMC
    em->report() << "DD edge: " << R->E.getNode() << "\n";
    em->report().flush();
    R->E.show(report.Freport(), 2);
#endif
    em->stopIO();
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
protected:
  static int order_policy;
  static const int ORDER_HIGH_TO_LOW = 0;
  static const int ORDER_LOW_TO_HIGH = 1;
  static const int ORDER_MODEL = 2;
  friend void InitializeSaturationMeddly(exprman* em);

  int* event_order;
  int event_order_size;
public:
  meddly_implicitgen();
  virtual ~meddly_implicitgen();
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual void RunEngine(hldsm* m, result &states_only); 

protected:
  virtual void generateRSS(meddly_varoption &x, timer &w) = 0;
  virtual const char* getAlgName() const = 0;

  // Called before we begin state generation
  virtual void initGen() { };

  // Called after we are done state generation
  virtual void doneGen() { };

  virtual void reportGen(bool err, DisplayStream &s) const { };

  void buildNextStateFunc(meddly_varoption &x) const;
  
  void preprocess(dsde_hlm &m);

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
    em->report() << " using Meddly: ";
    em->report() << getAlgName() << " algorithm, ";
    em->report() << getStyleName() << " vars.\n";
    return true;
  }

  inline bool stopGen(bool err, const hldsm &hm, 
                              const timer &w) const {
    return meddly_procgen::stopGen(err, hm.Name(), "reachability set", w);
  }

  void buildRSS(meddly_varoption &x);

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
}

meddly_implicitgen::~meddly_implicitgen()
{
  free(event_order);
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
  
  meddly_states* rss = 0;
  meddly_varoption* mvo = 0;
  try {
    dsde_hlm* dhm = smart_cast <dsde_hlm*> (hm);
    DCASSERT(dhm);
    preprocess(*dhm);
    initGen();
    rss = new meddly_states;
    mvo = makeVariableOption(*dhm, *rss);
    DCASSERT(mvo);
    buildRSS(*mvo);
    doneGen();
  }
  catch (subengine::error e) {
    doneGen();
    Delete(rss);
    delete mvo;
    hm->SetProcess(MakeErrorModel());
    throw e;
  }

  subengine* finisher = 0;
  if (hm->GetProcessType() == lldsm::FSM) {
    finisher = new mxd_fsm_finish(usePotentialEdges(), mvo);
    lm = StartMeddlyFSM(rss);
  } else {
    finisher = new mxd_mc_finish(usePotentialEdges(), mvo);
    lm = StartMeddlyMC(rss);
  }
  hm->SetProcess(lm);
  lm->setCompletionEngine(finisher);

  if (states_only.getBool())  return;
  if (0==finisher)            throw No_Engine;

  finisher->RunEngine(hm, states_only);
}

void meddly_implicitgen::buildNextStateFunc(meddly_varoption &x) const
{
  DCASSERT(x.ms.mxd_wrap);

  if (debug.startReport()) {
    debug.report() << "Updating event DDs\n";
    debug.stopIO();
  }
  x.updateEvents(debug, x.ms.mxd_wrap, 0);

  meddly_encoder& mxd = *(x.ms.mxd_wrap);
  const dsde_hlm &m = x.parent;

  shared_ddedge* N = smart_cast<shared_ddedge*>(mxd.makeEdge(0));
  DCASSERT(N);
  mxd.buildSymbolicConst(false, N);

  for (long e=0; e<m.getNumEvents(); e++) {
    MEDDLY::dd_edge enable = x.getEventEnabling(e);

#ifdef DEBUG_DETAILS
    if (debug.startReport()) {
      debug.report() << "Enabling DD for event ";
      debug.report() << m.readEvent(e)->Name();
      debug.report() << " DD edge: " << enable.getNode() << "\n";
      debug.report().flush();
      enable.show(debug.Freport(), 2);
      debug.stopIO();
    }
#endif

    MEDDLY::dd_edge firing = x.getEventFiring(e);

#ifdef DEBUG_DETAILS
    if (debug.startReport()) {
      debug.report() << "Next-state DD for event ";
      debug.report() << m.readEvent(e)->Name();
      debug.report() << " DD edge: " << firing.getNode() << "\n";
      debug.report().flush();
      firing.show(debug.Freport(), 2);
      debug.stopIO();
    }
#endif

    // TBD: deal with priority

    // "AND" together the enabling and next state.
    firing *= enable;

#ifdef DEBUG_EVENT_NSFS
    if (debug.startReport()) {
      debug.report() << "(final) next-state DD for event ";
      debug.report() << m.readEvent(e)->Name();
      debug.report() << " DD edge: " << firing.getNode() << "\n";
      debug.report().flush();
      firing.show(debug.Freport(), 2);
      debug.stopIO();
    }
#endif

    // Add this to overall next-state function  
    N->E += firing;
  } // for e
 
  Delete(x.ms.nsf);
  x.ms.nsf = N;
}


void meddly_implicitgen::preprocess(dsde_hlm &m) 
{
  // Check partition 
  if (!m.hasPartInfo()) {
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
    em->cout() << m.getEvent(event_order[e])->Name() << " ";
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
  if (startGen(x.parent)) {
    em->stopIO();
  }

  //
  // Build the initial state set, and other initializations
  //
  if (report.startReport()) {
    em->report() << "Initializing forests\n";
    em->stopIO();
  }

  try {
    x.initializeVars();

    if (report.startReport()) {
      em->report() << "Initialized  forests, took ";
      em->report() << watch.elapsed_seconds() << " seconds\n";
      em->stopIO();
    }

    x.initializeEvents(debug);

    // 
    // Build next-state function
    //
    if (report.startReport()) {
      em->report() << "Building next-state function\n";
      subwatch.reset();
      em->stopIO();
    }

    buildNextStateFunc(x);

    if (report.startReport()) {
      em->report() << "Built    next-state function, took ";
      em->report() << subwatch.elapsed_seconds() << " seconds\n";
  #ifdef DEBUG_FINAL_NSF
      em->report() << "DD edge: " << x.ms.getNSF().getNode() << "\n";
      em->report().flush();
      x.ms.getNSF().show(em->Freport(), 2);
      em->report() << "Initial state: " << x.ms.getInitial().getNode() << "\n";
      em->report().flush();
      x.ms.getInitial().show(em->Freport(), 2);
  #endif
  #ifdef DEBUG_REFCOUNTS
      em->report() << "Forest:\n";
      em->report().flush();
      x.ms.mxd_wrap->getForest()->showInfo(em->Freport(), 1);
      fflush(em->Freport());
  #endif
      em->stopIO();
    }

    DCASSERT(x.ms.nsf);

    //
    // Generate reachability set
    //
    if (report.startReport()) {
      em->report() << "Building reachability set\n";
      em->stopIO();
      subwatch.reset();
    }

    generateRSS(x, subwatch);

    if (report.startReport()) {
      em->report() << "Built    reachability set, took ";
      em->report() << subwatch.elapsed_seconds() << " seconds\n";
      em->stopIO();
    }

    if (stopGen(false, x.parent, watch)) {
      reportGen(false, em->report());
      x.reportStats(em->report());
      em->stopIO();
    }
  } // try

  catch (subengine::error status) {
    if (stopGen(true, x.parent, watch)) em->stopIO();
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
    shared_ddedge* S = x.ms.newMddEdge();
    MEDDLY::apply(
      MEDDLY::REACHABLE_STATES_DFS,
      x.ms.getInitial(), 
      x.ms.getNSF(),
      S->E
    );
    x.ms.setStates(S);
    checkTerm("Generation failed", x.parent);
  }
  catch (MEDDLY::error ce) {
    convert(ce, "Generation failed", x.parent);
  }
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
    shared_ddedge* S = x.ms.newMddEdge();
    MEDDLY::apply(
      MEDDLY::REACHABLE_STATES_BFS,
      x.ms.getInitial(), 
      x.ms.getNSF(),
      S->E
    );
    x.ms.setStates(S);
    return checkTerm("Generation failed", x.parent);
  }
  catch (MEDDLY::error ce) {
    return convert(ce, "Generation failed", x.parent);
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
  MEDDLY::dd_edge F = x.ms.getInitial();
  shared_ddedge* S = x.ms.newMddEdge();
  S->E = x.ms.getInitial();
  while (F.getNode()) {
    iterations++;
    if (debug.startReport()) {
      debug.report() << "Starting iteration ";
      debug.report().Put(iterations, 5);
      debug.report() << ":\n";
      debug.stopIO();
    }
    // compute N(F)
    try {
      MEDDLY::apply(MEDDLY::POST_IMAGE, F, x.ms.getNSF(), F);
      checkTerm("post-image", x.parent);
    }
    catch (MEDDLY::error ce) {
      convert(ce, "post-image", x.parent);
    }
    if (debug.startReport()) {
      debug.report() << "\tdone F:=N(F)\n";
      debug.stopIO();
    }
    // subtract S
    try {
      MEDDLY::apply(MEDDLY::DIFFERENCE, F, S->E, F);
      checkTerm("set difference", x.parent);
    }
    catch (MEDDLY::error ce) {
      convert(ce, "set difference", x.parent);
    }
    if (debug.startReport()) {
      debug.report() << "\tdone F:=F-S  ";
      double card = F.getCardinality();
      debug.report().Put(card, 13);
      debug.report() << " states in frontier set\n";
      debug.stopIO();
    }
    // add F to S
    try {
      MEDDLY::apply(
        MEDDLY::UNION, S->E, F, S->E
      );
      checkTerm("set union", x.parent);
    }
    catch (MEDDLY::error ce) {
      convert(ce, "set union", x.parent);
    }
    if (debug.startReport()) {
      debug.report() << "\tdone S:=S+F  ";
      double card = S->E.getCardinality();
      debug.report().Put(card, 13);
      debug.report() << " reachable states so far";
      debug.newLine();
      long nodes = x.ms.getMddForest()->getCurrentNumNodes();
      debug.report() << nodes << " nodes in forest, ";
      debug.report() << w.elapsed_seconds() << " seconds total time\n";
      debug.stopIO();
    }
  } // while F
  x.ms.setStates(S);
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
  MEDDLY::dd_edge Old(x.ms.getMddForest());
  x.ms.getMddForest()->createEdge(false, Old);
  x.ms.states->E = x.ms.getInitial();
  while (x.ms.states->E != Old) {
    iterations++;
    if (debug.startReport()) {
      debug.report() << "Starting iteration ";
      debug.report().Put(iterations, 5);
      debug.report() << ":\n";
      debug.stopIO();
    }
    Old = x.ms.states->E;
    // compute S = N(S)
    try {
      MEDDLY::apply(MEDDLY::POST_IMAGE, 
                    x.ms.states->E, x.ms.getNSF(), x.ms.states->E);
      checkTerm("post-image", x.parent);
    }
    catch (MEDDLY::error ce) {
      convert(ce, "post-image", x.parent);
    }
    if (debug.startReport()) {
      debug.report() << "\tdone S':=N(S)\n";
      debug.stopIO();
    }
    // compute S = Old + S
    try {
      MEDDLY::apply(MEDDLY::UNION, 
                  x.ms.states->E, Old, x.ms.states->E);
      checkTerm("set union", x.parent);
    }
    catch (MEDDLY::error ce) {
      convert(ce, "set union", x.parent);
    }
    if (debug.startReport()) {
      debug.report() << "\tdone S:=S+S'  ";
      double card = x.ms.states->E.getCardinality();
      debug.report().Put(card, 13);
      debug.report() << " reachable states so far";
      debug.newLine();
      long nodes = x.ms.getMddForest()->getCurrentNumNodes();
      debug.report() << nodes << " nodes in forest, ";
      debug.report() << w.elapsed_seconds() << " seconds total time\n";
      debug.stopIO();
    }
  } // while F
}

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeSaturationMeddly(exprman* em)
{
  if (0==em) return;

  RegisterEngine(em,
    "MeddlyProcessGeneration",
    "SATURATION",
    "The Saturation algorithm, as implemented in Meddly",
    &the_meddly_saturation
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

}
