
// $Id$

#include "gen_exp_as.h"
#include "gen_rg_base.h"

#include "../Timers/timers.h"

// Formalisms and such
#define DSDE_HLM_DETAILS
#include "../Formlsms/dsde_hlm.h"
#include "../Formlsms/mc_llm.h"
#include "../Formlsms/fsm_llm.h"

// Modules
#include "../Modules/expl_states.h"

// Generation templates
#include "gen_index.h"

// External libs
#include "statelib.h"
#include "graphlib.h"
#include "mclib.h"
#include "lslib.h"



// **************************************************************************
// *                                                                        *
// *                         indexed_statedbs class                         *
// *                                                                        *
// **************************************************************************

struct indexed_statedbs {
  StateLib::state_db &tandb;
  StateLib::state_db &vandb;
  long tan_unexp;
  long van_unexp;
public:
  indexed_statedbs(StateLib::state_db &tdb, StateLib::state_db &vdb);
  // required
  inline bool addVanishing(shared_state* s, long &index) {
    long oldsize = vandb.Size();
    index = vandb.InsertState(s->readState(), s->getStateSize());
    return index >= oldsize;
  }
  inline bool addTangible(shared_state* s, long &index) {
    long oldsize = tandb.Size();
    index = tandb.InsertState(s->readState(), s->getStateSize());
    return index >= oldsize;
  }
  inline bool hasUnexploredVanishing() const {
    return van_unexp < vandb.Size();
  }
  inline bool hasUnexploredTangible() const {
    return tan_unexp < tandb.Size();
  }
  inline long getUnexploredVanishing(shared_state* s) {
    DCASSERT(s);
    vandb.GetStateKnown(van_unexp, s->writeState(), s->getStateSize());
    return van_unexp++;
  }
  inline long getUnexploredTangible(shared_state* s) {
    DCASSERT(s);
    tandb.GetStateKnown(tan_unexp, s->writeState(), s->getStateSize());
    return tan_unexp++;
  }
  inline void clearVanishing() {
    vandb.Clear();
    van_unexp = 0;
  }
  inline void eliminateVanishing() {
    clearVanishing();
  }
  inline static bool statesOnly() {
    return true;
  }
  // required but not used
  inline static void addInitial(long) { 
    DCASSERT(0); 
    throw subengine::Engine_Failed;
  }
  inline static void addInitialVanishing(long, double) { 
    DCASSERT(0); 
    throw subengine::Engine_Failed;
  }
  inline static void addInitialTangible(long, double) { 
    DCASSERT(0); 
    throw subengine::Engine_Failed;
  }
  inline static void addEdge(long, long) { 
    DCASSERT(0); 
    throw subengine::Engine_Failed;
  }
  inline static void addVVEdge(long, long, double) { 
    DCASSERT(0);
    throw subengine::Engine_Failed;
  }
  inline static void addVTEdge(long, long, double) { 
    DCASSERT(0); 
    throw subengine::Engine_Failed;
  }
  inline static void addTVEdge(long, long, double) { 
    DCASSERT(0); 
    throw subengine::Engine_Failed;
  }
  inline static void addTTEdge(long, long, double) { 
    DCASSERT(0); 
    throw subengine::Engine_Failed;
  }
};


indexed_statedbs
::indexed_statedbs(StateLib::state_db &tdb, StateLib::state_db &vdb)
 : tandb(tdb), vandb(vdb) 
{
  tan_unexp = van_unexp = 0;
}

// **************************************************************************
// *                                                                        *
// *                        indexed_reachgraph class                        *
// *                                                                        *
// **************************************************************************

struct indexed_reachgraph : public indexed_statedbs {
  GraphLib::digraph &rg;
  ObjectList <long> *initlist;
public:
  indexed_reachgraph(StateLib::state_db &tdb, StateLib::state_db &vdb, GraphLib::digraph &rg);
  ~indexed_reachgraph();
  // required
  inline static bool statesOnly() {
    return false;
  }
  inline void addInitial(long st) {
    DCASSERT(initlist);
    initlist->Append(st);
  }
  inline void enlarge(long st) {
    if (st < rg.getNumNodes()) return;
    rg.addNodes(st+1-rg.getNumNodes());
  }
  inline void addEdge(long from, long to) {
    try {
      enlarge(MAX(from, to));
      rg.addEdge(from, to);
    }
    catch (GraphLib::error e) {
      throw convert(e);
    }
  }
  // handy
  inline void finish() {
    try {
      enlarge(tandb.Size()-1);
    }
    catch (GraphLib::error e) {
      throw convert(e);
    }
  }
  inline subengine::error convert(GraphLib::error e) const {
    switch (e.getCode()) {
      case GraphLib::error::Out_Of_Memory:  return subengine::Out_Of_Memory;
      default:                              return subengine::Engine_Failed;
    }
  }
  inline void exportInitial(LS_Vector &s0) const {
    if (initlist) {
      s0.size = initlist->Length();
      s0.index = initlist->CopyAndClear();
    }
  }
};


indexed_reachgraph
::indexed_reachgraph(StateLib::state_db &tdb, StateLib::state_db &vdb, GraphLib::digraph &therg)
 :indexed_statedbs(tdb, vdb), rg(therg)
{
  initlist = new ObjectList <long>;
}

indexed_reachgraph::~indexed_reachgraph()
{
  delete initlist;
}

// **************************************************************************
// *                                                                        *
// *                           indexed_smp  class                           *
// *                                                                        *
// **************************************************************************

struct indexed_smp : public indexed_statedbs {
  hldsm &model;
  LS_Options &vansolver;
  MCLib::vanishing_chain &smp;
public:
  indexed_smp(hldsm &m, StateLib::state_db &tdb, StateLib::state_db &vdb, 
              LS_Options &vs, MCLib::vanishing_chain &smp);
  // handy
  inline static int status2int(MCLib::error status) {
    switch (status.getCode()) {
      case MCLib::error::Out_Of_Memory:    return -2;
      case MCLib::error::Bad_Index:        return -1;
      default:                             return -4;
    }
  }
  inline subengine::error convert(MCLib::error e, const char* x) {
    switch (e.getCode()) {
      case MCLib::error::Out_Of_Memory:
          return subengine::Out_Of_Memory;

      default:
          if (model.StartError(0)) {
            if (x) {
              model.SendError("Couldn't ");
              model.SendError(x);
              model.SendError(" in process: ");
            } else {
              model.SendError("While building process: ");
            }
            model.SendError(e.getString());
            model.DoneError();
          }
          return subengine::Engine_Failed;
    } // switch e
  }
  inline void exportInitial(LS_Vector &s0) {
    try {
      smp.getInitialVector(s0);
    }
    catch (MCLib::error e) {
      throw convert(e, "build initial vector");
    }
  }

  // required
  inline bool addVanishing(shared_state* s, long &index) {
    index = vandb.InsertState(s->readState(), s->getStateSize());
    if (index < smp.getNumVanishing()) return false;
    try {
      CHECK_RETURN(smp.addVanishing(), index);
    } 
    catch (MCLib::error e) {
      index = status2int(e);
    }
    return true;
  }
  inline bool addTangible(shared_state* s, long &index) {
    index = tandb.InsertState(s->readState(), s->getStateSize());
    if (index < smp.getNumTangible()) return false;
    try {
      CHECK_RETURN(smp.addTangible(), index);
    }
    catch (MCLib::error e) {
      index = status2int(e);
    }
    return true;
  }
  inline static bool statesOnly() {
    return false;
  }
  inline void eliminateVanishing() {
    clearVanishing();
    try {
      smp.eliminateVanishing(vansolver);
    }
    catch (MCLib::error e) {
      throw convert(e, "eliminate vanishings");
    }
  }
  inline void addInitialVanishing(long st, double wt) {
    try {
      smp.addInitialVanishing(st, wt);
    }
    catch (MCLib::error e) {
      throw convert(e, "add initial state");
    }
  }
  inline void addInitialTangible(long st, double wt) {
    try {
      smp.addInitialTangible(st, wt);
    }
    catch (MCLib::error e) {
      throw convert(e, "add initial state");
    }
  }
  inline void addTTEdge(long from, long to, double wt) {
    try {
      smp.addTTedge(from, to, wt);
    }
    catch (MCLib::error e) {
      throw convert(e, "add edge");
    }
  }
  inline void addTVEdge(long from, long to, double wt) {
    try {
      smp.addTVedge(from, to, wt);
    }
    catch (MCLib::error e) {
      throw convert(e, "add edge");
    }
  }
  inline void addVTEdge(long from, long to, double wt) {
    try {
      smp.addVTedge(from, to, wt);
    }
    catch (MCLib::error e) {
      throw convert(e, "add edge");
    }
  }
  inline void addVVEdge(long from, long to, double wt) {
    try {
      smp.addVVedge(from, to, wt);
    }
    catch (MCLib::error e) {
      throw convert(e, "add edge");
    }
  }
};


indexed_smp
::indexed_smp(hldsm &m, StateLib::state_db &tdb, StateLib::state_db &vdb, 
              LS_Options &vs, MCLib::vanishing_chain &the_smp)
 : indexed_statedbs(tdb, vdb), model(m), vansolver(vs), smp(the_smp)
{
}


// **************************************************************************
// *                                                                        *
// *                            as_procgen class                            *
// *                                                                        *
// **************************************************************************

class as_procgen : public process_generator {
  const exp_state_lib* statelib;
public:
  as_procgen(const exp_state_lib* sl);
  
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual void RunEngine(hldsm* m, result &);

protected:
  /** Build the reachability set and (maybe) graph.
        @param  m   High-level model.
        @param  rss State space.  If "static", it is assumed to be known.
        @param  s0  Initial states; will be filled in IF rg is not 0.
        @param  rg  Graph goes here.  If 0, we only generate states!

        @throw  Appropriate error code.
  */
  void generateRG(dsde_hlm* m, StateLib::state_db* rss, LS_Vector &s0, GraphLib::digraph* rg) const;

  /** Build the reachability set and (maybe) Markov chain.
        @param  m   High-level model.
        @param  ss  State space.  If "static", it is assumed to be known.
        @param  s0  Initial distribution; will be filled in IF smp is not 0.
        @param  smp Vanishing chain goes here.  If 0, we only generate states!

        @throw  Appropriate error code.
  */
  void generateMC(dsde_hlm* m, StateLib::state_db* ss, LS_Vector &s0, MCLib::vanishing_chain* smp) const;


  inline void initial_distro(const LS_Vector &init) const {
    if (!debug.startReport()) return;
    debug.report() << "Built initial distribution [";
    for (long z=0; z<init.size; z++) {
      DCASSERT(init.index);
      DCASSERT(init.f_value);
      if (z) debug.report() << ", ";
      debug.report() << init.index[z] << ":" << init.f_value[z];
    }
    debug.report() << "]\n";
    debug.stopIO();
  }
};

// **************************************************************************
// *                           as_procgen methods                           *
// **************************************************************************

as_procgen::as_procgen(const exp_state_lib* sl)
 : process_generator()
{
  statelib = sl;
}

bool as_procgen::AppliesToModelType(hldsm::model_type mt) const
{
  return (hldsm::Asynch_Events == mt);
}

void as_procgen::RunEngine(hldsm* hm, result &statesonly)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));
  lldsm* lm = hm->GetProcess();
  if (lm) {
    subengine* e = lm->getCompletionEngine();
    if (0==e)                 return;
    if (statesonly.getBool()) return;
    if (e!=this)              return e->RunEngine(hm, statesonly);
  }
  StateLib::state_db* rss = 0;
  GraphLib::digraph* rg = 0;
  MCLib::Markov_chain* mc = 0;
  MCLib::vanishing_chain* vc = 0;
  const char* the_proc = 0;
  
  bool nondeterm = (hm->GetProcessType() == lldsm::FSM);

  if (nondeterm) {
    if (!statesonly.getBool()) {
      rg = new GraphLib::digraph(true); 
    }
    if (lm) {
      rss = GrabExplicitFSMStates(lm);
      DCASSERT(rg);
      the_proc = "reachability graph";
    } else {
      rss = statelib->createStateDB(true, false);
      if (rg) the_proc = "reachability set & graph";
      else    the_proc = "reachability set";
    }
  } else {
    if (lm) {
      rss = GrabExplicitMCStates(lm);
      the_proc = "Markov chain";
    } else {
      rss = statelib->createStateDB(true, false);
      if (statesonly.getBool()) the_proc = "reachability set";
      else                      the_proc = "reachability set & Markov chain";
    }
    DCASSERT(rss);
    if (!statesonly.getBool()) {
      vc = MCLib::startVanishingChain(false, rss->Size(), 0);
    }
  }

  DCASSERT(rss);
  
  // Start reporting on generation
  timer* watch = 0;
  if (startGen(*hm, the_proc)) {
    if (!rss->IsStatic())
        em->report() << " using " << statelib->getDBMethod();
    em->report() << "\n";
    em->stopIO();
    watch = makeTimer();
  }

  // set initial distribution
  LS_Vector init;
  init.size = 0;
  init.index = 0;
  init.f_value = 0;
  init.d_value = 0;

  // Generate process
  dsde_hlm* dsm = smart_cast <dsde_hlm*> (hm);
  DCASSERT(dsm);
  em->waitTerm();
  bool procOK = true;
  error bailOut;
  try {
    if (nondeterm)  generateRG(dsm, rss, init, rg);
    else            generateMC(dsm, rss, init, vc);
  }
  catch (error e) {
    procOK = false;
    bailOut = e;
  }

  if (vc) {
    mc = vc->grabTTandClear();
    DCASSERT(mc);
    delete vc;
  }

  // Report on generation
  if (stopGen(procOK, hm->Name(), the_proc, watch)) {
    if (!rss->IsStatic()) {
      em->report().Put('\t');
      em->report().PutMemoryCount(rss->ReportMemTotal(), 3);
      em->report() << " required for state space construction\n";
      em->report() << "\t" << rss->Size() << " states generated\n";
    } 
    if (rg) {
      em->report().Put('\t');
      em->report().PutMemoryCount(rg->ReportMemTotal(), 3);
      em->report() << " required for reachability graph construction\n";
      em->report() << "\t" << rg->getNumEdges() << " graph edges\n";
    } 
    if (mc) {
      em->report().Put('\t');
      em->report().PutMemoryCount(mc->ReportMemTotal(), 3);
      em->report() << " required for Markov chain construction\n";
      em->report() << "\t" << mc->getNumArcs() << " Markov chain edges\n";
    }
    em->stopIO();
  }
  em->resumeTerm();

  // Did we succeed so far?
  if (!procOK) {
    delete rg;
    delete mc;
    if (lm) {
      lm->setCompletionEngine(0);
    } else {
      delete rss;
      hm->SetProcess(MakeErrorModel());
    }
    doneTimer(watch);
    throw bailOut;
  }

  // Set process as known so far
  if (0==lm) {
    if (nondeterm) {
      lm = StartExplicitFSM(rss);
    } else {
      lm = StartExplicitMC(false, rss);
    }
    hm->SetProcess(lm); 
  }

  // Neat trick! Specify how to finish building the process:
  DCASSERT(lm);
  if (statesonly.getBool()) {
    lm->setCompletionEngine(this);
    doneTimer(watch);
    return;
  }

  // Start reporting on compaction
  if (startCompact(*hm, the_proc)) {
    em->report() << "\n";
    em->stopIO();
    watch->reset();
  }

  // Compact and finish process
  if (nondeterm) {
    FinishExplicitFSM(lm, init, rg);
  } else {
    FinishExplicitMC(lm, init, mc);
  }
  
  // Report on compaction
  if (stopCompact(hm->Name(), the_proc, watch, lm)) {
    em->stopIO();
    doneTimer(watch);
  }

  // We've generated the entire process now.
  lm->setCompletionEngine(0);
}


void as_procgen::generateRG(dsde_hlm* dsm, StateLib::state_db* tandb, 
  LS_Vector &s0, GraphLib::digraph* rg) const
{
  DCASSERT(dsm);
  DCASSERT(tandb);

  StateLib::state_db* vandb = statelib->createStateDB(true, false);

  if (rg) {
    indexed_reachgraph myrg(*tandb, *vandb, *rg);
    generateIndexedRG(debug, *dsm, myrg);
    myrg.exportInitial(s0);
    myrg.finish();
  } else {
    indexed_statedbs myrs(*tandb, *vandb);
    generateIndexedRG(debug, *dsm, myrs);
  }

  delete vandb;
}


void as_procgen::generateMC(dsde_hlm* dsm, StateLib::state_db* tandb, 
  LS_Vector &s0, MCLib::vanishing_chain* smp) const
{
  DCASSERT(dsm);
  DCASSERT(tandb);

  // TBD: put these options elsewhere
  LS_Options vansolver;
  vansolver.method = LS_Gauss_Seidel;

  StateLib::state_db* vandb = statelib->createStateDB(true, false);

  if (smp) {
    indexed_smp mysmp(*dsm, *tandb, *vandb, vansolver, *smp);
    generateIndexedSMP(debug, *dsm, mysmp);
    mysmp.exportInitial(s0);
    initial_distro(s0);
  } else {
    indexed_statedbs myrs(*tandb, *vandb);
    generateIndexedSMP(debug, *dsm, myrs);
  }

  delete vandb;
}



// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeExplicitAsynchGenerators(exprman* em)
{
  if (0==em) return;

  // Initialize state library
  const exp_state_lib* sl = InitExplicitStateStorage(em);

  // Register engines
  RegisterSubengine(em, 
    "ProcessGeneration",
    "EXPLICIT",
    new as_procgen(sl)
  );
}



