
// $Id$

#include "satur_meddly.h"

#define PROC_MEDDLY_DETAILS
#include "proc_meddly.h"

#include "../Timers/timers.h"
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
  virtual error RunEngine(hldsm* m, result &statesonly);
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

subengine::error mxd_fsm_finish::RunEngine(hldsm* hm, result &states_only)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));
  DCASSERT(hm->GetProcessType() == lldsm::FSM);
  lldsm* lm = hm->GetProcess();
  DCASSERT(lm);
  subengine* e = lm->getCompletionEngine();
  if (0==e)                   return Success;
  if (states_only.getBool())  return Success;
  if (e!=this)                return e->RunEngine(hm, states_only);

  meddly_states* rss = GrabMeddlyFSMStates(lm);
  DCASSERT(rss);

  timer* watch = makeTimer();
  if (report.startReport()) {
    em->report() << "Finishing FSM using Meddly\n";
    em->report() << "\tUsing ";
    em->report() << ( potential ? "potential edges\n" : "actual edges\n" );
    em->stopIO();
  }

  rss->proc_wrap = Share(rss->mxd_wrap);
  
  sv_encoder::error se = FinishMeddlyFSM(lm, potential);
  if (se) if (hm->StartError(0)) {
    em->cerr() << "Couldn't build actual edges: ";
    em->cerr() << sv_encoder::getNameOfError(se);
    em->newLine();
    em->cerr() << "Using potential instead";
    hm->DoneError();
  }

  if (report.startReport()) {
    em->report() << "Finished  FSM using Meddly, took ";
    em->report() << watch->elapsed() << " seconds\n";
    em->stopIO();
  }
  doneTimer(watch);

  lm->setCompletionEngine(0);
  delete this;
  return Success;
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
  virtual error RunEngine(hldsm* m, result &statesonly);
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

subengine::error mxd_mc_finish::RunEngine(hldsm* hm, result &states_only)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));
  DCASSERT(hm->GetProcessType() == lldsm::CTMC);
  lldsm* lm = hm->GetProcess();
  DCASSERT(lm);
  subengine* e = lm->getCompletionEngine();
  if (0==e)                   return Success;
  if (states_only.getBool())  return Success;
  if (e!=this)                return e->RunEngine(hm, states_only);

  timer* watch = makeTimer();
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
  MEDDLY::forest* newf = rss->vars->createForest(
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


  sv_encoder::error se = FinishMeddlyMC(lm, R, potential);
  if (se) if (hm->StartError(0)) {
    em->cerr() << "Couldn't build actual edges: ";
    em->cerr() << sv_encoder::getNameOfError(se);
    em->newLine();
    em->cerr() << "Using potential instead";
    hm->DoneError();
  }
  Delete(R);

  if (report.startReport()) {
    em->report() << "Finished  CTMC using Meddly, took ";
    em->report() << watch->elapsed() << " seconds\n";
    rss->proc_wrap->reportStats(em->report());
#ifdef DEBUG_FINAL_CTMC
    em->report() << "DD edge: " << R->E.getNode() << "\n";
    em->report().flush();
    R->E.show(report.Freport(), 2);
#endif
    em->stopIO();
  }
  doneTimer(watch);


  lm->setCompletionEngine(0);
  delete this;
  return Success;
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
  virtual error RunEngine(hldsm* m, result &states_only); 

protected:
  virtual error generateRSS(meddly_varoption &x, timer* w) = 0;
  virtual const char* getAlgName() const = 0;

  // Called before we begin state generation
  virtual void initGen() { };

  // Called after we are done state generation
  virtual void doneGen() { };

  virtual void reportGen(bool err, DisplayStream &s) const { };

  error buildNextStateFunc(meddly_varoption &x) const;
  
  error preprocess(dsde_hlm &m);

  inline static error checkTerm(const char* errstr, const hldsm &hm) {
    if (!em->caughtTerm()) return Success;
    if (hm.StartError(0)) {
      em->cerr() << "signal caught during " << errstr;
      hm.DoneError();
    }
    return Terminated;
  }

  inline static error convert(MEDDLY::error ce, const char* errstr,
                              const hldsm &hm) 
  {
    if (hm.StartError(0)) {
      em->cerr() << errstr << " in Meddly with error code:";
      em->newLine();
      em->cerr() << ce.getName();
      hm.DoneError();
    }
    switch (ce.getCode()) {
      case MEDDLY::error::INSUFFICIENT_MEMORY:  return Out_Of_Memory;
      default:                                  return Engine_Failed;
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
                              const timer* w) const {
    return meddly_procgen::stopGen(err, hm.Name(), "reachability set", w);
  }

  error buildRSS(meddly_varoption &x);

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

subengine::error meddly_implicitgen::RunEngine(hldsm* hm, result &states_only)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));
  lldsm* lm = hm->GetProcess();
  if (lm) {
    // we already have something, deal with it
    subengine* e = lm->getCompletionEngine();
    if (0==e)                   return Success;
    if (states_only.getBool())  return Success;
    if (e!=this)                return e->RunEngine(hm, states_only);
    // Shouldn't get here
    DCASSERT(0);
    return Engine_Failed;
  } 
  
  dsde_hlm* dhm = smart_cast <dsde_hlm*> (hm);
  DCASSERT(dhm);
  error status = preprocess(*dhm);
  if (Success != status) {
    hm->SetProcess(MakeErrorModel());
    return status;
  }
  initGen();
  meddly_states* rss = new meddly_states;
  meddly_varoption* mvo = makeVariableOption(*dhm, *rss);
  DCASSERT(mvo);
  status = buildRSS(*mvo);
  doneGen();
  if (Success != status) {
    Delete(rss);
    delete mvo;
    hm->SetProcess(MakeErrorModel());
    return status;
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

  if (states_only.getBool())  return Success;
  if (0==finisher)            return No_Engine;

  return finisher->RunEngine(hm, states_only);
}

subengine::error
meddly_implicitgen::buildNextStateFunc(meddly_varoption &x) const
{
  DCASSERT(x.ms.mxd_wrap);

  if (debug.startReport()) {
    debug.report() << "Updating event DDs\n";
    debug.stopIO();
  }
  error status = x.updateEvents(debug, x.ms.mxd_wrap, 0);
  if (status) return status;

  meddly_encoder& mxd = *(x.ms.mxd_wrap);
  const dsde_hlm &m = x.parent;

  shared_ddedge* N = smart_cast<shared_ddedge*>(mxd.makeEdge(0));
  DCASSERT(N);
  CHECK_RETURN(
      mxd.buildSymbolicConst(false, N), 
      sv_encoder::Success 
  );

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
  return status;
}


subengine::error 
meddly_implicitgen::preprocess(dsde_hlm &m) 
{
  // Check partition 
  if (!m.hasPartInfo()) {
    if (m.StartError(0)) {
      em->cerr() << "Saturation requires a structured model (try partitioning)";
      m.DoneError();
    }
    return Engine_Failed;
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
      return Out_Of_Memory;
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
      return Engine_Failed;
  };

#ifdef DEBUG_DEPENDENCIES
  em->cout() << "Using event order:\n\t" << event_order[0]->Name();
  for (int e=1; e<m.getNumEvents(); e++) {
    em->cout() << ", " << event_order[e]->Name();
  }
  em->cout() << "\n";
#endif
  return Success;
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


subengine::error 
meddly_implicitgen::buildRSS(meddly_varoption &x)
{
  error status;
  timer* watch = 0;
  timer* subwatch = 0;
  if (startGen(x.parent)) {
    em->stopIO();
    watch = makeTimer();
  }

  //
  // Build the initial state set, and other initializations
  //
  if (report.startReport()) {
    em->report() << "Initializing forests\n";
    subwatch = makeTimer();
    em->stopIO();
  }

  status = x.initializeVars();

  if (status) {
    if (stopGen(true, x.parent, watch)) em->stopIO();
    doneTimer(watch);
    doneTimer(subwatch);
    return status;
  } 
  if (report.startReport()) {
    em->report() << "Initialized  forests, took ";
    em->report() << watch->elapsed() << " seconds\n";
    em->stopIO();
  }

  status = x.initializeEvents(debug);

  if (status) {
    if (stopGen(true, x.parent, watch)) em->stopIO();
    doneTimer(watch);
    return status;
  } 
  
  // 
  // Build next-state function
  //
  if (report.startReport()) {
    em->report() << "Building next-state function\n";
    subwatch->reset();
    em->stopIO();
  }

  status = buildNextStateFunc(x);

  if (report.startReport()) {
    em->report() << "Built    next-state function, took ";
    em->report() << subwatch->elapsed() << " seconds\n";
#ifdef DEBUG_FINAL_NSF
    em->report() << "DD edge: " << x.ms.nsf->E.getNode() << "\n";
    em->report().flush();
    x.ms.nsf->E.show(em->Freport(), 2);
    em->report() << "Initial state: " << x.ms.initial->E.getNode() << "\n";
    em->report().flush();
    x.ms.initial->E.show(em->Freport(), 2);
#endif
#ifdef DEBUG_REFCOUNTS
    em->report() << "Forest:\n";
    em->report().flush();
    x.ms.mxd_wrap->getForest()->showInfo(em->Freport(), 1);
    fflush(em->Freport());
#endif
    em->stopIO();
  }

  if (status) {
    if (stopGen(true, x.parent, watch)) em->stopIO();
    doneTimer(watch);
    doneTimer(subwatch);
    return status;
  }
  DCASSERT(x.ms.nsf);
  
  //
  // Generate reachability set
  //
  if (report.startReport()) {
    em->report() << "Building reachability set\n";
    em->stopIO();
    subwatch->reset();
  }
  DCASSERT(0==x.ms.states);
  x.ms.states = new shared_ddedge(x.ms.mdd_wrap->getForest());

  status = generateRSS(x, subwatch);

  if (report.startReport()) {
    em->report() << "Built    reachability set, took ";
    em->report() << subwatch->elapsed() << " seconds\n";
    em->stopIO();
  }

  if (stopGen(status != Success, x.parent, watch)) {
    reportGen(status != Success, em->report());
    x.reportStats(em->report());
    em->stopIO();
  }

  return status;
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
  virtual error generateRSS(meddly_varoption &x, timer* w);
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

subengine::error 
meddly_saturation::generateRSS(meddly_varoption &x, timer*)
{
  try {
    MEDDLY::apply(
      MEDDLY::REACHABLE_STATES_DFS,
      x.ms.initial->E, 
      x.ms.nsf->E,
      x.ms.states->E
    );
    return checkTerm("Generation failed", x.parent);
  }
  catch (MEDDLY::error ce) {
    return convert(ce, "Generation failed", x.parent);
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
  virtual error generateRSS(meddly_varoption &x, timer* w);
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

subengine::error 
meddly_traditional::generateRSS(meddly_varoption &x, timer*)
{
  try {
    MEDDLY::apply(
      MEDDLY::REACHABLE_STATES_BFS,
      x.ms.initial->E, 
      x.ms.nsf->E,
      x.ms.states->E
    );
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
  virtual error generateRSS(meddly_varoption &x, timer* w);
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

subengine::error 
meddly_frontier::generateRSS(meddly_varoption &x, timer* w)
{
  error status = Success;

  MEDDLY::dd_edge F = x.ms.initial->E;
  x.ms.states->E = x.ms.initial->E;
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
      MEDDLY::apply(MEDDLY::POST_IMAGE, F, x.ms.nsf->E, F);
      status = checkTerm("post-image", x.parent);
    }
    catch (MEDDLY::error ce) {
      status = convert(ce, "post-image", x.parent);
    }
    if (status) break;
    if (debug.startReport()) {
      debug.report() << "\tdone F:=N(F)\n";
      debug.stopIO();
    }
    // subtract S
    try {
      MEDDLY::apply(MEDDLY::DIFFERENCE, F, x.ms.states->E, F);
      status = checkTerm("set difference", x.parent);
    }
    catch (MEDDLY::error ce) {
      status = convert(ce, "set difference", x.parent);
    }
    if (status) break;
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
        MEDDLY::UNION, x.ms.states->E, F, x.ms.states->E
      );
      status = checkTerm("set union", x.parent);
    }
    catch (MEDDLY::error ce) {
      status = convert(ce, "set union", x.parent);
    }
    if (status) break;
    if (debug.startReport()) {
      DCASSERT(w);
      debug.report() << "\tdone S:=S+F  ";
      double card = x.ms.states->E.getCardinality();
      debug.report().Put(card, 13);
      debug.report() << " reachable states so far";
      debug.newLine();
      long nodes = x.ms.mdd_wrap->getForest()->getCurrentNumNodes();
      debug.report() << nodes << " nodes in forest, ";
      debug.report() << w->elapsed() << " seconds total time\n";
      debug.stopIO();
    }
  } // while F
  return status;
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
  virtual error generateRSS(meddly_varoption &x, timer* w);
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

subengine::error 
meddly_nextall::generateRSS(meddly_varoption &x, timer* w)
{
  error status = Success;

  MEDDLY::dd_edge Old(x.ms.mdd_wrap->getForest());
  x.ms.mdd_wrap->getForest()->createEdge(false, Old);
  x.ms.states->E = x.ms.initial->E;
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
                    x.ms.states->E, x.ms.nsf->E, x.ms.states->E);
      status = checkTerm("post-image", x.parent);
    }
    catch (MEDDLY::error ce) {
      status = convert(ce, "post-image", x.parent);
    }
    if (status) break;
    if (debug.startReport()) {
      debug.report() << "\tdone S':=N(S)\n";
      debug.stopIO();
    }
    // compute S = Old + S
    try {
      MEDDLY::apply(MEDDLY::UNION, 
                  x.ms.states->E, Old, x.ms.states->E);
      status = checkTerm("set union", x.parent);
    }
    catch (MEDDLY::error ce) {
      status = convert(ce, "set union", x.parent);
    }
    if (status) break;
    if (debug.startReport()) {
      DCASSERT(w);
      debug.report() << "\tdone S:=S+S'  ";
      double card = x.ms.states->E.getCardinality();
      debug.report().Put(card, 13);
      debug.report() << " reachable states so far";
      debug.newLine();
      long nodes = x.ms.mdd_wrap->getForest()->getCurrentNumNodes();
      debug.report() << nodes << " nodes in forest, ";
      debug.report() << w->elapsed() << " seconds total time\n";
      debug.stopIO();
    }
  } // while F
  return status;
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
