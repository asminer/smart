
#include "gen_exp_as.h"
#include "gen_rg_base.h"

#include "../ExprLib/startup.h"

// Formalisms and such
#include "../Formlsms/dsde_hlm.h"
#include "../Formlsms/rss_expl.h"
#include "../Formlsms/rgr_grlib.h"
#include "../Formlsms/proc_mclib.h"

// Modules
#include "../Modules/expl_states.h"

// Generation templates
#include "gen_templ.h"

// External libs
#include "../_StateLib/statelib.h"
#include "../_GraphLib/graphlib.h"
#include "../_MCLib/mclib.h"
#include "../_LSLib/lslib.h"
#include "../_Timer/timerlib.h"


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
  static inline void show(OutputStream &s, bool van, long id, const shared_state* curr_st)
  {
    if (van) {
      s << "vanishing state# ";
    } else {
      s << "tangible  state# ";
    }
    s.Put(id, 4);
    s << " : ";
    curr_st->Print(s, 0);
  }
  static inline void show(OutputStream &s, long id)
  {
    s << " state# " << id;
  }
  static inline void show(OutputStream &s, const shared_state* curr_st)
  {
    s << " state ";
    curr_st->Print(s, 0);
  }

  static inline void makeIllegalID(long &id) {
    id = -1;
  }
  inline bool add(bool van, const shared_state* s, long &id) {
    StateLib::state_db &db = van ? vandb : tandb;
    long oldsize = db.Size();
    id = db.InsertState(s->readState(), s->getStateSize());
    return id >= oldsize;
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
  inline void clearVanishing(named_msg &debug) {
    if (debug.startReport()) {
      debug.report() << "Eliminating vanishing states\n";
      debug.stopIO();
    }
    vandb.Clear();
    van_unexp = 0;
  }
  inline void eliminateVanishing(named_msg &debug) {
    clearVanishing(debug);
  }
  inline static bool statesOnly() {
    return true;
  }
  // required but should do nothing
  inline static void addInitial(long) { }
  inline static void addInitial(bool, long, double) { }
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
  GraphLib::dynamic_digraph &rg;
  ObjectList <long> *initlist;
public:
  indexed_reachgraph(StateLib::state_db &tdb, StateLib::state_db &vdb, GraphLib::dynamic_digraph &rg);
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
::indexed_reachgraph(StateLib::state_db &tdb, StateLib::state_db &vdb, GraphLib::dynamic_digraph &therg)
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
  inline void convert(MCLib::error e, const char* x) {
    switch (e.getCode()) {
      case MCLib::error::Out_Of_Memory:
          throw subengine::Out_Of_Memory;

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
          throw subengine::Engine_Failed;
    } // switch e
  }
  inline void exportInitial(LS_Vector &s0) {
    try {
      // smp.getInitialVector(s0);
      smp.buildInitialVector(false, s0);  // use doubles?
    }
    catch (MCLib::error e) {
      convert(e, "build initial vector");
    }
  }

  // required
  inline bool add(bool van, const shared_state* s, long &id) {
    if (van) return addVanishing(s, id);
    else     return addTangible(s, id);
  }
  inline bool addVanishing(const shared_state* s, long &index) {
    index = vandb.InsertState(s->readState(), s->getStateSize());
    if (index < smp.getNumVanishing()) return false;
    try {
      smp.addVanishing();
    } 
    catch (MCLib::error e) {
      convert(e, "add vanishing state");
    }
    return true;
  }
  inline bool addTangible(const shared_state* s, long &index) {
    index = tandb.InsertState(s->readState(), s->getStateSize());
    if (index < smp.getNumTangible()) return false;
    try {
      smp.addTangible();
    }
    catch (MCLib::error e) {
      convert(e, "add tangible state");
    }
    return true;
  }
  inline static bool statesOnly() {
    return false;
  }
  inline void eliminateVanishing(named_msg &debug) {
    clearVanishing(debug);
    try {
      smp.eliminateVanishing(vansolver);
    }
    catch (MCLib::error e) {
      convert(e, "eliminate vanishings");
    }
  }
  inline void addInitial(bool van, long st, double wt) {
    try {
      if (van)  smp.addInitialVanishing(st, wt);
      else      smp.addInitialTangible(st, wt);
    }
    catch (MCLib::error e) {
      convert(e, "add initial state");
    }
  }
  /*
  inline void addInitialVanishing(long st, double wt) {
    try {
      smp.addInitialVanishing(st, wt);
    }
    catch (MCLib::error e) {
      convert(e, "add initial state");
    }
  }
  inline void addInitialTangible(long st, double wt) {
    try {
      smp.addInitialTangible(st, wt);
    }
    catch (MCLib::error e) {
      convert(e, "add initial state");
    }
  }
  */
  inline void addTTEdge(long from, long to, double wt) {
    try {
      smp.addTTedge(from, to, wt);
    }
    catch (MCLib::error e) {
      convert(e, "add edge");
    }
  }
  inline void addTVEdge(long from, long to, double wt) {
    try {
      smp.addTVedge(from, to, wt);
    }
    catch (MCLib::error e) {
      convert(e, "add edge");
    }
  }
  inline void addVTEdge(long from, long to, double wt) {
    try {
      smp.addVTedge(from, to, wt);
    }
    catch (MCLib::error e) {
      convert(e, "add edge");
    }
  }
  inline void addVVEdge(long from, long to, double wt) {
    try {
      smp.addVVedge(from, to, wt);
    }
    catch (MCLib::error e) {
      convert(e, "add edge");
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
  void generateRG(dsde_hlm* m, StateLib::state_db* rss, LS_Vector &s0, GraphLib::dynamic_digraph* rg) const;

  /** Build the reachability set and (maybe) Markov chain.
        @param  m   High-level model.
        @param  ss  State space.  If "static", it is assumed to be known.
        @param  s0  Initial distribution; will be filled in IF smp is not 0.
        @param  smp Vanishing chain goes here.  If 0, we only generate states!

        @throw  Appropriate error code.
  */
  void generateMC(dsde_hlm* m, StateLib::state_db* ss, LS_Vector &s0, MCLib::vanishing_chain* smp) const;


  inline void initial_distro(const LS_Vector &init) const {
    if (!Debug().startReport()) return;
    Debug().report() << "Built initial distribution [";
    for (long z=0; z<init.size; z++) {
      DCASSERT(init.index);
      DCASSERT(init.f_value);
      if (z) Debug().report() << ", ";
      Debug().report() << init.index[z] << ":" << init.f_value[z];
    }
    Debug().report() << "]\n";
    Debug().stopIO();
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
  GraphLib::dynamic_digraph* rg = 0;
  MCLib::vanishing_chain* vc = 0;
  const char* the_proc = 0;
  
  bool nondeterm = (hm->GetProcessType() == lldsm::FSM);

  state_lldsm* slm = dynamic_cast <state_lldsm*> (lm);
  if (lm && (0==slm)) {
    hm->StartError(0);
    hm->SendError("Couldn't complete process, unknown type for partial process");
    hm->DoneError();
    lm->setCompletionEngine(0);
    return;
  }

  if (nondeterm) {
    if (!statesonly.getBool()) {
      rg = new GraphLib::dynamic_digraph(true); 
    }
    if (slm) {
      rss = slm->getRSS()->getStateDatabase();
      DCASSERT(rg);
      the_proc = "reachability graph";
    } else {
      rss = statelib->createStateDB(true, false);
      if (rg) the_proc = "reachability set & graph";
      else    the_proc = "reachability set";
    }
  } else {
    if (slm) {
      rss = slm->getRSS()->getStateDatabase();
      the_proc = "Markov chain";
    } else {
      rss = statelib->createStateDB(true, false);
      if (statesonly.getBool()) the_proc = "reachability set";
      else                      the_proc = "reachability set & Markov chain";
    }
    DCASSERT(rss);
    if (!statesonly.getBool()) {
      // vc = MCLib::startVanishingChain(false, rss->Size(), 0);
      vc = new MCLib::vanishing_chain(false, rss->Size(), 0);
    }
  }

  DCASSERT(rss);
  
  // Start reporting on generation
  timer watch;
  if (startGen(*hm, the_proc)) {
    if (!rss->IsStatic())
        Report().report() << " using " << statelib->getDBMethod();
    Report().report() << "\n";
    Report().stopIO();
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
  error bailOut = Engine_Failed;
  try {
    if (nondeterm)  generateRG(dsm, rss, init, rg);
    else            generateMC(dsm, rss, init, vc);
  }
  catch (error e) {
    procOK = false;
    bailOut = e;
  }

  // Report on generation
  if (stopGen(!procOK, *hm, the_proc, watch)) {
    if (!rss->IsStatic()) {
      Report().report().Put('\t');
      Report().report().PutMemoryCount(rss->ReportMemTotal(), 3);
      Report().report() << " required for state space construction\n";
      Report().report() << "\t" << rss->Size() << " states generated\n";
    } 
    if (rg) {
      Report().report().Put('\t');
      Report().report().PutMemoryCount(rg->getMemTotal(), 3);
      Report().report() << " required for reachability graph construction\n";
      Report().report() << "\t" << rg->getNumEdges() << " graph edges\n";
    } 
    if (vc) {
      Report().report().Put('\t');
      Report().report().PutMemoryCount(vc->getMemTotal(), 3);
      Report().report() << " required for Markov chain construction\n";
      Report().report() << "\t" << vc->TT().getNumEdges() << " Markov chain edges\n";
    }
    Report().stopIO();
  }
  em->resumeTerm();

  // Did we succeed so far?
  if (!procOK) {
    delete rg;
    delete vc;
    if (lm) {
      lm->setCompletionEngine(0);
    } else {
      delete rss;
      hm->SetProcess(MakeErrorModel());
    }
    throw bailOut;
  }

  // Set process as known so far
  if (0==slm) {
    if (nondeterm) {
      slm = new graph_lldsm(lldsm::FSM);
    } else {
      slm = new stochastic_lldsm(lldsm::CTMC);
    }
    slm->setRSS( new expl_reachset(rss) );
    lm = slm;
    hm->SetProcess(lm); 
  }

  // Neat trick! Specify how to finish building the process:
  DCASSERT(lm);
  if (statesonly.getBool()) {
    lm->setCompletionEngine(this);
    return;
  }

  // Start reporting on compaction
  if (startCompact(*hm, the_proc)) {
    Report().report() << "\n";
    Report().stopIO();
    watch.reset();
  }

  // Compact and finish process
  if (nondeterm) {
    graph_lldsm* glm = smart_cast <graph_lldsm*>(lm);
    DCASSERT(glm);
    grlib_reachgraph* erg = new grlib_reachgraph(rg);
    erg->setInitial(init);
    glm->setRGR(erg);
  } else {
    stochastic_lldsm* glm = smart_cast <stochastic_lldsm*>(lm);
    DCASSERT(glm);
    mclib_process* mcp = new mclib_process(vc);
    glm->setPROC(init, mcp);
  }
  
  // Report on compaction
  if (stopCompact(hm->Name(), the_proc, watch, lm)) {
    Report().stopIO();
  }

  // We've generated the entire process now.
  lm->setCompletionEngine(0);
}


void as_procgen::generateRG(dsde_hlm* dsm, StateLib::state_db* tandb, 
  LS_Vector &s0, GraphLib::dynamic_digraph* rg) const
{
  DCASSERT(dsm);
  DCASSERT(tandb);

  StateLib::state_db* vandb = statelib->createStateDB(true, false);

  if (rg) {
    indexed_reachgraph myrg(*tandb, *vandb, *rg);
    generateRGt<indexed_reachgraph, long>(Debug(), *dsm, myrg);
    myrg.exportInitial(s0);
    myrg.finish();
  } else {
    indexed_statedbs myrs(*tandb, *vandb);
    generateRGt<indexed_statedbs, long>(Debug(), *dsm, myrs);
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

    switch (remove_vanishing) {
      case BY_PATH:
        generateMCt<indexed_smp, long>(Debug(), *dsm, mysmp);
        break;

      case BY_SUBGRAPH:
        generateSMPt<indexed_smp, long>(Debug(), *dsm, mysmp);
        break;

      default:
        DCASSERT(0);
    }
    mysmp.exportInitial(s0);
    initial_distro(s0);

  } else {

    indexed_statedbs myrs(*tandb, *vandb);

    switch (remove_vanishing) {
      case BY_PATH:
        generateMCt<indexed_statedbs, long>(Debug(), *dsm, myrs);
        break;

      case BY_SUBGRAPH:
        generateSMPt<indexed_statedbs, long>(Debug(), *dsm, myrs);
        break;

      default:
        DCASSERT(0);
    }
  }

  delete vandb;
}



// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_asynchgen : public initializer {
  public:
    init_asynchgen();
    virtual bool execute();
};
init_asynchgen the_asynchgen_initializer;

init_asynchgen::init_asynchgen() : initializer("init_asynchgen")
{
  usesResource("em");
  usesResource("engtypes");
}

bool init_asynchgen::execute()
{
  if (0==em) return false;

  // Initialize state library
  const exp_state_lib* sl = InitExplicitStateStorage(em);

  // Register engines
  RegisterSubengine(em, 
    "ProcessGeneration",
    "EXPLICIT",
    new as_procgen(sl)
  );

  return true;
}



