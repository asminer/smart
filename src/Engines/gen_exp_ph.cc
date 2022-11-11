
#include "gen_exp_ph.h"
#include "gen_rg_base.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/mod_vars.h"

// Formalisms and such
#include "../Formlsms/phase_hlm.h"
#include "../Formlsms/rss_expl.h"
#include "../Formlsms/proc_mclib.h"

// Modules
#include "../Modules/expl_states.h"

// External libs
#include "../_StateLib/statelib.h"
#include "../_MCLib/mclib.h"
#include "../_LSLib/lslib.h"
#include "../_Timer/timerlib.h"

// **************************************************************************
// *                                                                        *
// *                          phase_procgen  class                          *
// *                                                                        *
// **************************************************************************

class phase_procgen : public process_generator {
  const exp_state_lib* statelib;
public:
  phase_procgen(const exp_state_lib* sl);

  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual void RunEngine(hldsm* m, result &);

protected:
  void generateMC(phase_hlm* m, LS_Vector &s0, long &acc, long &trap,
    StateLib::state_db* rss, MCLib::vanishing_chain* smp) const;

  void lostStateError(hldsm* m, const shared_state* st) const;
  void badWeightError(hldsm* m, const char* what, const result& x) const;
  void MCError(hldsm* m, const char* what, MCLib::error e) const;
  inline void terminateError() const {
    if (em->startError()) {
      em->causedBy(0);
      em->cerr() << "Process construction prematurely terminated";
      em->stopIO();
    }
    throw Terminated;
  }
  inline void initial(bool tang, long num, const shared_state* st) const {
    if (!Debug().startReport()) return;
    Debug().report() << "Adding initial";
    if (tang)   Debug().report() << " tangible  state# ";
    else        Debug().report() << " vanishing state# ";
    Debug().report().Put(num, 4);
    Debug().report() << " : ";
    st->Print(Debug().report(), 0);
    Debug().report() << "\n";
    Debug().stopIO();
  }
  inline void exploring(bool tang, long num, const shared_state* st) const {
    if (!Debug().startReport()) return;
    Debug().report() << "Exploring";
    if (tang)   Debug().report() << " tangible  state# ";
    else        Debug().report() << " vanishing state# ";
    Debug().report().Put(num, 4);
    Debug().report() << " : ";
    st->Print(Debug().report(), 0);
    Debug().report() << "\n";
    Debug().stopIO();
  }
  inline void reached(bool tang, long num, double rate, const shared_state* st) const {
    if (!Debug().startReport()) return;
    if (rate) {
      Debug().report() << " (";
      Debug().report().Put(rate, 4);
      Debug().report() << ")";
    }
    Debug().report() << " --> ";
    st->Print(Debug().report(), 0);
    if (tang)  Debug().report() << " (tangible  index ";
    else  Debug().report() << " (vanishing index ";
    Debug().report().Put(num, 4);
    Debug().report() << ")\n";
    Debug().stopIO();
  }
  inline void eliminating(long count) const {
    if (!Debug().startReport()) return;
    Debug().report() << "Eliminating " << count << " vanishing states\n";
    Debug().stopIO();
  }
  inline bool addVan(hldsm* m, MCLib::vanishing_chain* vc, long i)
  const {
    if (i < vc->getNumVanishing()) return false;
    try {
      vc->addVanishing();
      return true;
    }
    catch (MCLib::error status) {
      MCError(m, "add vanishing state to", status);
    }
    return true;
  }
  inline bool addTan(hldsm* m, MCLib::vanishing_chain* vc, long i)
  const {
    if (i < vc->getNumTangible()) return false;
    try {
      vc->addTangible();
      return true;
    }
    catch (MCLib::error status) {
      MCError(m, "add tangible state to", status);
    }
    return true;
  }
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
// *                         phase_procgen  methods                         *
// **************************************************************************

phase_procgen::phase_procgen(const exp_state_lib* sl)
 : process_generator()
{
  statelib = sl;
}

bool phase_procgen::AppliesToModelType(hldsm::model_type mt) const
{
  return (hldsm::Phase_Type == mt);
}

void phase_procgen::RunEngine(hldsm* hm, result &statesonly)
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
  phase_hlm* phm = smart_cast <phase_hlm*> (hm);
  DCASSERT(phm);
  StateLib::state_db* rss = 0;
  const char* the_proc = 0;
  MCLib::vanishing_chain* vc = 0;

  stochastic_lldsm* slm = dynamic_cast <stochastic_lldsm*> (lm);
  if (lm && (0==slm)) {
    hm->StartError(0);
    hm->SendError("Couldn't complete process, unknown type for partial process");
    hm->DoneError();
    lm->setCompletionEngine(0);
    return;
  }

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
    // vc = MCLib::startVanishingChain(phm->isDiscrete(), rss->Size(), 0);
    vc = new MCLib::vanishing_chain(phm->isDiscrete(), rss->Size(), 0);
  }

  // Start reporting on generation
  timer watch;
  if (startGen(*hm, the_proc)) {
    if (!rss->IsStatic())
        em->report() << " using " << statelib->getDBMethod();
    em->report() << "\n";
    em->stopIO();
  }

  // set initial distribution
  LS_Vector init;
  init.size = 0;
  init.index = 0;
  init.f_value = 0;
  init.d_value = 0;

  // Generate process
  long accept = -1;
  long trap = -1;
  em->waitTerm();
  bool procOK = true;
  error bailOut = Engine_Failed;
  try {
    generateMC(phm, init, accept, trap, rss, vc);
  }
  catch (error e) {
    procOK = false;
    bailOut = e;
  }

  // Report on generation
  if (stopGen(!procOK, *hm, the_proc, watch)) {
    if (!rss->IsStatic()) {
      em->report().Put('\t');
      em->report().PutMemoryCount(rss->ReportMemTotal(), 3);
      em->report() << " required for state space construction\n";
      em->report() << "\t" << rss->Size() << " states generated\n";
    }
    if (vc) {
      em->report().Put('\t');
      em->report().PutMemoryCount(vc->getMemTotal(), 3);
      em->report() << " required for Markov chain construction\n";
      em->report() << "\t" << vc->TT().getNumEdges() << " Markov chain edges\n";
    }
    em->stopIO();
  }
  em->resumeTerm();

  // Did we succeed so far?
  if (!procOK) {
    delete vc;
    if (lm) {
      lm->setCompletionEngine(0);
    } else {
      delete rss;
      hm->SetProcess(MakeErrorModel());
    }
    throw bailOut;
  }

  // Start reporting on compaction
  if (startCompact(*hm, the_proc)) {
    em->report() << "\n";
    em->stopIO();
    watch.reset();
  }

  // Compact states and finish process, if necessary
  if (0==slm) {
    slm = new stochastic_lldsm(phm->isDiscrete() ? lldsm::DTMC : lldsm::CTMC);
    slm->setRSS( new expl_reachset(rss) );
    lm = slm;
    hm->SetProcess(lm);
  }
  mclib_process* mcp = new mclib_process(vc);
  mcp->setAcceptState(accept);
  mcp->setTrapState(trap);
  slm->setPROC(init, mcp);

  // Report on compaction
  if (stopCompact(hm->Name(), the_proc, watch, lm)) {
    em->stopIO();
  }

  // Neat trick! Specify how to finish building the process:
  DCASSERT(lm);
  if (statesonly.getBool()) {
    lm->setCompletionEngine(this);
  } else {
    lm->setCompletionEngine(0);
  }
}


void phase_procgen::generateMC(phase_hlm* dsm, LS_Vector &init, long &accept,
  long &trap, StateLib::state_db* tandb, MCLib::vanishing_chain* smp) const
{
  DCASSERT(dsm);
  DCASSERT(tandb);

  // flatten
  int svs = 0;
  dsm->reindexStateVars(svs);
  DCASSERT(dsm->NumStateVars() == svs);

  // TBD: put these options elsewhere
  LS_Options vansolver;
  vansolver.method = LS_Gauss_Seidel;

  // vanishing will be eliminated
  StateLib::state_db* vandb = statelib->createStateDB(true, false);

  const StateLib::state_coll* tan = tandb->GetStateCollection();
  const StateLib::state_coll* van = vandb->GetStateCollection();

  // allocate temporary states
  shared_state* curr_st = new shared_state(dsm);
  shared_state* next_st = new shared_state(dsm);
  int stsize = curr_st->getStateSize();

  try {

    // Find and insert the initial state
    dsm->getInitialState(curr_st);
    if (dsm->isVanishingState(curr_st)) {
      long ind = vandb->InsertState(curr_st->readState(), stsize);
      initial(false, ind, curr_st);
      if (smp) {
        addVan(dsm, smp, ind);
        smp->addInitialVanishing(ind, 1.0);
      }
    } else {
      long ind = tandb->InsertState(curr_st->readState(), stsize);
      initial(true, ind, curr_st);
      if (smp) {
        addTan(dsm, smp, ind);
        smp->addInitialTangible(ind, 1.0);
      }
    }

    long v_exp = 0;
    long t_exp = 0;
    bool current_is_vanishing = false;
    // Combined tangible + vanishing explore loop!
    for (;;) {

      // Check for sigterm
      if (em->caughtTerm()) {
        terminateError();
      }

      // Get next state to explore, with priority to vanishings.
      if (v_exp < van->Size()) {
        // explore next vanishing
        van->GetStateKnown(v_exp, curr_st->writeState(), stsize);
        current_is_vanishing = true;
        exploring(false, v_exp, curr_st);
      } else {
        // No unexplored vanishing; safe to trash them, if desired
        if (current_is_vanishing) { // assumes we want to eliminate vanishing...
          eliminating(van->Size());
          try {
            smp->eliminateVanishing(vansolver);
          }
          catch (MCLib::error vc_status) {
            MCError(dsm, "eliminate vanishings in", vc_status);
          }
          vandb->Clear();
          v_exp = 0;
        }
        current_is_vanishing = false;
        // find next tangible to explore; if none, break out
        if (t_exp < tan->Size()) {
          tan->GetStateKnown(t_exp, curr_st->writeState(), stsize);
          exploring(true, t_exp, curr_st);
        } else {
          break;
        }
      }

      // Look at outgoing edges from this state
      long num_edges = dsm->setSourceState(curr_st);
      for (long e=0; e<num_edges; e++) {
        double r = dsm->getOutgoingFromSource(e, next_st);
        if (r<=0) continue;
        long newindex = -2;
        bool next_is_vanishing = dsm->isVanishingState(next_st);
        if (next_is_vanishing) {
          // new state is vanishing
          newindex = vandb->InsertState(next_st->readState(), stsize);
          reached(false, newindex, r, next_st);
          if (smp) addVan(dsm, smp, newindex);
        } else {
          // new state is tangible
          newindex = tandb->InsertState(next_st->readState(), stsize);
          reached(true, newindex, r, next_st);
          if (smp) addTan(dsm, smp, newindex);
        }
        if (newindex < 0) {
          if (-1 == newindex) lostStateError(dsm, next_st);
          else                throw Out_Of_Memory;
        }

        if (0==smp) continue;

        // Add appropriate edge to vanishing chain
        try {
          if (current_is_vanishing)
              if (next_is_vanishing)
                  smp->addVVedge(v_exp, newindex, r);
              else
                  smp->addVTedge(v_exp, newindex, r);
          else
              if (next_is_vanishing)
                  smp->addTVedge(t_exp, newindex, r);
              else
                  smp->addTTedge(t_exp, newindex, r);
        }
        catch (MCLib::error vc_status) {
          MCError(dsm, "add edge to", vc_status);
          break;
        }
      } // for e

      // advance appropriate ptr
      if (current_is_vanishing) v_exp++; else t_exp++;
    } // while

    // Build initial distribution if necessary
    if (smp) {
      try {
        smp->buildInitialVector(true, init);  // floats
        initial_distro(init);
      }
      catch (MCLib::error vc_status) {
        MCError(dsm, "build initial vector for", vc_status);
      }

      // get index for accepting state
      dsm->getAcceptingState(curr_st);
      accept = tandb->FindState(curr_st->readState(), stsize);

      // get index for trap state
      dsm->getTrapState(curr_st);
      trap = tandb->FindState(curr_st->readState(), stsize);
    }

    // Cleanup:
    delete vandb;
    Delete(curr_st);
    Delete(next_st);
  } // try

  catch (subengine::error e) {
    // Cleanup
    delete vandb;
    Delete(curr_st);
    Delete(next_st);
    throw e;
  }
}


void phase_procgen::lostStateError(hldsm* m, const shared_state* s) const
{
  DCASSERT(m);
  DCASSERT(s);
  if (m->StartError(0)) {
    em->cerr() << " Couldn't find reachable state ";
    s->Print(em->cerr(), 0);
    em->cerr() << " during process generation";
    m->DoneError();
  }
  throw Engine_Failed;
}

void phase_procgen
::badWeightError(hldsm* m, const char* what, const result& x) const
{
  DCASSERT(m);
  if (m->StartError(0)) {
    em->cerr() << "Bad " << what << ": ";
    DCASSERT(em->REAL);
    em->REAL->print(em->cerr(), x);
    em->cerr() << " during process generation";
    m->DoneError();
  }
  throw Engine_Failed;
}

void phase_procgen::MCError(hldsm* m, const char* what, MCLib::error e) const
{
  DCASSERT(m);
  if (e.getCode() == MCLib::error::Out_Of_Memory) throw Out_Of_Memory;
  // what kind of error is this?
  if (m->StartError(0)) {
    em->cerr() << "Couldn't " << what << " process: ";
    em->cerr() << e.getString();
    m->DoneError();
  }
  throw Engine_Failed;
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_phasegen : public initializer {
  public:
    init_phasegen();
    virtual bool execute();
};
init_phasegen the_phasegen_initializer;

init_phasegen::init_phasegen() : initializer("init_phasegen")
{
  usesResource("em");
  usesResource("engtypes");
}

bool init_phasegen::execute()
{
  if (0==em) return false;

  // Initialize state library
  const exp_state_lib* sl = InitExplicitStateStorage(em);

  // Register engines
  RegisterSubengine(em,
    "ProcessGeneration",
    "EXPLICIT",
    new phase_procgen(sl)
  );
//  Register Coverability engines
  RegisterSubengine(em,
    "ProcessGeneration",
    "EXPLICITCOV",
    new phase_procgen(sl)
  );

  return true;
}
