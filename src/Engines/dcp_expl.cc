
#include "dcp_expl.h"

#include "../Streams/textfmt.h"
#include "../Options/options.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/mod_inst.h"
#include "../ExprLib/measures.h"
#include "../ExprLib/engine.h"

// Formalisms and such
#include "../Formlsms/noevnt_hlm.h"

// External libs
#include "../_StateLib/statelib.h"
#include "../_Timer/timerlib.h"

// **************************************************************************
// *                                                                        *
// *                            state_lib  class                            *
// *                                                                        *
// **************************************************************************

class icp_state_lib : public library {
public:
  icp_state_lib();
  virtual const char* getVersionString() const {
    return StateLib::LibraryVersion();
  }
  virtual bool hasFixedPointer() const {
    return false;
  }
  virtual void printCopyright(doc_formatter* df) const;
};

icp_state_lib::icp_state_lib() : library(false, false)
{
}

void icp_state_lib::printCopyright(doc_formatter* df) const
{
  df->begin_indent();
  df->Out() << "State library copyright info here\n";
  df->end_indent();
}

// ******************************************************************
// *                                                                *
// *                     expl_states_only class                     *
// *                                                                *
// ******************************************************************

class expl_states_only : public lldsm {
protected:
  StateLib::state_coll* expl;
public:
  expl_states_only(StateLib::state_coll* ex);
protected:
  virtual ~expl_states_only();
  virtual const char* getClassName() const { return "expl_states_only"; }
public:
  const StateLib::state_coll* GetExplicit() const { return expl; }
};

expl_states_only::expl_states_only(StateLib::state_coll* ex) : lldsm(RSS)
{
  expl = ex;
}

expl_states_only::~expl_states_only()
{
  delete expl;
}


// **************************************************************************
// *                                                                        *
// *                           icp_stategen class                           *
// *                                                                        *
// **************************************************************************

/// Explicit state generator for integer constraint models.
class icp_stategen : public subengine {
protected:
  static reporting_msg report;
  static debugging_msg debug;
  friend class init_dcpengines;
public:
  icp_stategen();
  virtual~ icp_stategen();

  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual void RunEngine(hldsm* m, result &);
private:
  void Generate_NE_rec(int k);
  // variables used by Generate_NE_rec
  int N;                // num vars
  int* bounds;          // bounds per variable
  int* current;         // current state
  no_event_model* nem;  // current model
  StateLib::state_coll* states;   // stored states
protected:
  // returns true if the report stream is open
  inline bool startGen(const char* name) {
    if (report.startReport()) {
      report.report() << "Generating reachability set for model " << name;
      return true;
    }
    return false;
  }
  // returns true if the report stream is open
  inline bool stopGen(bool err, const char* n, const timer& w,
                                                long mem, long ns) {
    if (report.startReport()) {
      if (err)  report.report() << "Incomplete";
      else      report.report() << "Generated ";
      report.report() << " reachability set for model " << n << "\n";
      report.report() << "\t" << w.elapsed_seconds() << " seconds ";
      if (err)  report.report() << "until error\n";
      else      report.report() << "required for generation\n";
      if (mem >= 0) {
        report.report().Put('\t');
        report.report().PutMemoryCount(mem, 3);
        if (err)  report.report() << " used before error\n";
        else      report.report() << " required for state generation\n";
      }
      if (ns >= 0) {
        report.report() << "\t" << ns << " states generated";
        if (err) report.report() << " before error";
        report.report() << "\n";
      }
      return true;
    }
    return false;
  }
  inline void terminateError() const {
    if (em->startError()) {
      em->noCause();
      em->cerr() << "Process construction prematurely terminated";
      em->stopIO();
    }
    throw Terminated;
  }
};
reporting_msg icp_stategen::report;
debugging_msg icp_stategen::debug;

icp_stategen the_icp_stategen;

// **************************************************************************
// *                          icp_stategen methods                          *
// **************************************************************************

icp_stategen::icp_stategen() : subengine()
{
}

icp_stategen::~icp_stategen()
{
}

bool icp_stategen::AppliesToModelType(hldsm::model_type mt) const
{
  return (hldsm::No_Events == mt);
}

void icp_stategen::RunEngine(hldsm* hm, result &)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));
  if (hm->GetProcess())  return;  // already has SS?

  nem = smart_cast <no_event_model*> (hm);
  DCASSERT(nem);
  DCASSERT(nem->NumVars()>0);

  timer watch;
  if (startGen(hm->Name())) {
    em->report().Put('\n');
    em->stopIO();
  }

  N = nem->NumVars();
  bounds = new int[N];
  current= new int[N];
  for (int i=0; i<N; i++) {
    model_statevar* mv = nem->GetVar(i);
    DCASSERT(mv->HasBounds());
    bounds[i] = mv->NumPossibleValues();
    current[i] = 0;
  }

  em->waitTerm();
  states = StateLib::CreateCollection(false, false);
  bool OK = true;
  error foo = Engine_Failed;
  try {
    Generate_NE_rec(0);
  }
  catch (error e) {
    OK = false;
    foo = e;
  }

  if (stopGen(!OK, hm->Name(), watch,
              states->ReportMemTotal(), states->Size())) {
    em->stopIO();
  }
  em->resumeTerm();

  delete[] current;
  delete[] bounds;

  if (OK) {
    hm->SetProcess(new expl_states_only(states));
  } else {
    hm->SetProcess(MakeErrorModel());
    throw foo;
  }
}

void icp_stategen::Generate_NE_rec(int k)
{
  if (k>=N) {
    // visit this state
    long index = states->AddState(current, N);
    if (index < 0) {
      if (nem->StartError(0)) {
        em->cerr() << "Out of memory when adding to state space";
        nem->DoneError();
      }
      throw Out_Of_Memory;
    }
    if (debug.startReport()) {
      debug.report() << "Valid state: ";
      debug.report().PutArray(current, N);
      debug.report() << "\n";
      debug.stopIO();
    }
    return;
  }
  for (current[k] = 0; current[k] < bounds[k]; current[k]++) {
    // Check for sigterm
    if (em->caughtTerm()) return terminateError();

    nem->GetVar(k)->SetToValueNumber(current[k]);
    if (nem->SatisfiesConstraintsAt(k)) {
      Generate_NE_rec(k+1);
    }
  }
}

// **************************************************************************
// *                                                                        *
// *                         icp_ss_analyzer  class                         *
// *                                                                        *
// **************************************************************************

// abstract base class for min, max, sat engines
class icp_ss_analyzer : public subengine {
  static engtype* SSGen;
  friend class init_dcpengines;
public:
  icp_ss_analyzer();
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual void SolveMeasure(hldsm* m, measure* what);
protected:
  virtual void SolveExplicit(no_event_model* nem,
    const StateLib::state_coll* sc, measure* what) = 0;
};

engtype* icp_ss_analyzer::SSGen = 0;

icp_ss_analyzer::icp_ss_analyzer() : subengine()
{
}

bool icp_ss_analyzer::AppliesToModelType(hldsm::model_type mt) const
{
  return (hldsm::No_Events == mt);
}

void icp_ss_analyzer::SolveMeasure(hldsm* hm, measure* what)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));

  result dummy;
#ifdef DEVELOPMENT_CODE
  dummy.setNull();
#endif
  if (!SSGen) throw No_Engine;
  SSGen->runEngine(hm, dummy);
  if (0==hm->GetProcess()) {
    throw Engine_Failed;
  }

  no_event_model* nem = smart_cast <no_event_model*> (hm);
  DCASSERT(nem);

  // Are we explicit?
  const expl_states_only* esom;
  esom = dynamic_cast <const expl_states_only*> (hm->GetProcess());
  if (esom) {
    const StateLib::state_coll* sc = esom->GetExplicit();
    return SolveExplicit(nem, sc, what);
  } // if esom

  // constraints are in the wrong format.
  // TBD: Print an error message.

  throw No_Engine;
}


// **************************************************************************
// *                                                                        *
// *                           icp_minimize class                           *
// *                                                                        *
// **************************************************************************

class icp_minimize : public icp_ss_analyzer {
public:
  icp_minimize();
protected:
  virtual void SolveExplicit(no_event_model* nem,
    const StateLib::state_coll* sc, measure* what);
};

icp_minimize the_icp_minimize;

icp_minimize::icp_minimize() : icp_ss_analyzer()
{
}

void icp_minimize ::SolveExplicit(no_event_model* nem,
  const StateLib::state_coll* sc, measure* what)
{
  DCASSERT(nem);
  DCASSERT(sc);
  DCASSERT(what);

  if (0==sc->Size()) {
    what->SetNull();
    return;
  }

  int N = nem->NumVars();
  int* current = new int[N];
  traverse_data x(traverse_data::Compute);
  result foo;
  x.answer = &foo;

  long min_st = -1;
  long next = sc->FirstHandle();
  double min = 1e100;
  for (long i=0; i<sc->Size(); i++) {
    long currindex = next;
    next = sc->GetStateKnown(next, current, N);
    DCASSERT(next > 0);
    nem->SetState(current);
    what->ComputeRHS(x);
    DCASSERT(foo.isNormal());
    if (foo.getReal() < min) {
      min = foo.getReal();
      min_st = currindex;
    }
  }

  foo.setReal(min);
  what->SetValue(foo);

  // Need option or something to decide how many optima to show here...
  sc->GetStateKnown(min_st, current, N);
  nem->SetState(current);
  em->cout() << "Minimum value " << min << " obtainted in state ";
  nem->ShowCurrentState(em->cout());
  em->cout() << "\n";

  delete[] current;
}



// **************************************************************************
// *                                                                        *
// *                           icp_maximize class                           *
// *                                                                        *
// **************************************************************************

class icp_maximize : public icp_ss_analyzer {
public:
  icp_maximize();
protected:
  virtual void SolveExplicit(no_event_model* nem,
    const StateLib::state_coll* sc, measure* what);
};

icp_maximize the_icp_maximize;

icp_maximize::icp_maximize() : icp_ss_analyzer()
{
}

void icp_maximize::SolveExplicit(no_event_model* nem,
  const StateLib::state_coll* sc, measure* what)
{
  DCASSERT(nem);
  DCASSERT(sc);
  DCASSERT(what);

  if (0==sc->Size()) {
    what->SetNull();
    return;
  }

  int N = nem->NumVars();
  int* current = new int[N];
  traverse_data x(traverse_data::Compute);
  result foo;
  x.answer = &foo;

  long max_st = -1;
  long next = sc->FirstHandle();
  double max = -1e100;
  for (long i=0; i<sc->Size(); i++) {
    long currindex = next;
    next = sc->GetStateKnown(next, current, N);
    DCASSERT(next > 0);
    nem->SetState(current);
    what->ComputeRHS(x);
    DCASSERT(foo.isNormal());
    if (foo.getReal() > max) {
      max = foo.getReal();
      max_st = currindex;
    }
  }

  foo.setReal(max);
  what->SetValue(foo);

  // Need option or something to decide how many optima to show here...
  sc->GetStateKnown(max_st, current, N);
  nem->SetState(current);
  em->cout() << "Maximum value " << max << " obtainted in state ";
  nem->ShowCurrentState(em->cout());
  em->cout() << "\n";

  delete[] current;
}



// **************************************************************************
// *                                                                        *
// *                         icp_satisfiable  class                         *
// *                                                                        *
// **************************************************************************

class icp_satisfiable : public icp_ss_analyzer {
public:
  icp_satisfiable();
protected:
  virtual void SolveExplicit(no_event_model* nem,
    const StateLib::state_coll* sc, measure* what);
};

icp_satisfiable the_icp_satisfiable;

icp_satisfiable::icp_satisfiable() : icp_ss_analyzer()
{
}

void icp_satisfiable::SolveExplicit(no_event_model* nem,
  const StateLib::state_coll* sc, measure* what)
{
  DCASSERT(nem);
  DCASSERT(sc);
  DCASSERT(what);

  int N = nem->NumVars();
  int* current = new int[N];
  traverse_data x(traverse_data::Compute);
  result foo;
  foo.setBool(false);
  x.answer = &foo;

  long next = sc->FirstHandle();
  for (long i=0; i<sc->Size(); i++) {
    next = sc->GetStateKnown(next, current, N);
    nem->SetState(current);
    what->ComputeRHS(x);
    DCASSERT(foo.isNormal());
    if (foo.getBool())  break;  // satisfiable, stop!
  }
  what->SetValue(foo);

  // Need option or something to decide how many to show here...
  if (foo.getBool()) {
    em->cout() << "Satisfiable in state ";
    nem->ShowCurrentState(em->cout());
    em->cout() << "\n";
  }

  delete[] current;
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_dcpengines : public initializer {
  public:
    init_dcpengines();
    virtual bool execute();
};
init_dcpengines the_dcpengine_initializer;

init_dcpengines::init_dcpengines() : initializer("init_dcpengines")
{
  usesResource("em");
  usesResource("engtypes");
}

bool init_dcpengines::execute()
{
  if (0==em) return false;

  // Initialize libraries
  static icp_state_lib state_lib_data;
  em->registerLibrary(&state_lib_data);

  // Initialize options
  option* report = em->findOption("Report");
  if (report) report->addChecklistItem(
    "explicit_dcp_gen",
    "When set, explicit reachability set performance is reported.",
    icp_stategen::report, false
  );

  option* debug = em->findOption("Debug");
  if (debug) debug->addChecklistItem(
    "explicit_dcp_gen",
    "When set, explicit reachability set generation details are displayed.",
    icp_stategen::debug, false
  );


  // Register engines
  icp_ss_analyzer::SSGen = em->findEngineType("ExplicitDCSolve");
  DCASSERT(icp_ss_analyzer::SSGen);
  RegisterEngine(
    icp_ss_analyzer::SSGen,
    "IN_ORDER",
    "All possible assignments are checked, in order; valid ones are saved.",
    &the_icp_stategen
  );
  RegisterEngine(em,
      "MinExpr",
      "EXPLICIT",
      "Generates assignments satisfying constraints, explicitly, then checks them all for the minimum value of the expression",
      &the_icp_minimize
  );
  RegisterEngine(em,
      "MaxExpr",
      "EXPLICIT",
      "Generates assignments satisfying constraints, explicitly, then checks them all for the maximum value of the expression",
      &the_icp_maximize
  );
  RegisterEngine(em,
      "SatExpr",
      "EXPLICIT",
      "Generates assignments satisfying constraints, explicitly, then checks them all until the expression is satisfied",
      &the_icp_satisfiable
  );

  return true;
}



