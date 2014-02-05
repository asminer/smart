
// $Id$

#include "dcp_expl.h"

#include "../Timers/timers.h"
#include "../Streams/streams.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/mod_inst.h"
#include "../ExprLib/measures.h"
#include "../ExprLib/engine.h"

// Formalisms and such
#include "../Formlsms/noevnt_hlm.h"

// External libs
#include "statelib.h"

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
  virtual void printCopyright(OutputStream &s) const;
};

icp_state_lib::icp_state_lib() : library(false)
{
}

void icp_state_lib::printCopyright(OutputStream &s) const
{
  s << "\tState library copyright info here\n";
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
  static named_msg report;
  static named_msg debug;
  friend void InitializeDCPEngines(exprman* em);
public:
  icp_stategen();
  virtual~ icp_stategen();

  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual error RunEngine(hldsm* m, result &);
private:
  error Generate_NE_rec(int k);
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
  inline bool stopGen(bool err, const char* n, const timer* w, 
                                                long mem, long ns) {
    if (report.startReport()) {
      if (err)  report.report() << "Incomplete";
      else      report.report() << "Generated ";
      report.report() << " reachability set for model " << n << "\n";
      if (w) {
        report.report() << "\t" << w->elapsed() << " seconds ";
        if (err)  report.report() << "until error\n";
        else      report.report() << "required for generation\n";
      }
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
  inline error terminateError() const {
    if (em->startError()) {
      em->noCause();
      em->cerr() << "Process construction prematurely terminated";
      em->stopIO();
    }
    return Terminated;
  }
};
named_msg icp_stategen::report;
named_msg icp_stategen::debug;

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

subengine::error icp_stategen::RunEngine(hldsm* hm, result &)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));
  if (hm->GetProcess())  return Success;  // already has SS?

  nem = smart_cast <no_event_model*> (hm);
  DCASSERT(nem);
  DCASSERT(nem->NumVars()>0);

  timer* watch = 0;
  if (startGen(hm->Name())) {
    em->report().Put('\n');
    em->stopIO();
    watch = makeTimer();
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
  error foo = Generate_NE_rec(0);

  if (stopGen(foo, hm->Name(), watch, 
              states->ReportMemTotal(), states->Size())) {
    em->stopIO();
    doneTimer(watch);
  }
  em->resumeTerm();

  delete[] current;
  delete[] bounds;

  if (Success == foo) {
    hm->SetProcess(new expl_states_only(states));
  } else {
    hm->SetProcess(MakeErrorModel());
  }
  return foo; 
}

icp_stategen::error icp_stategen::Generate_NE_rec(int k)
{
  if (k>=N) {
    // visit this state
    long index = states->AddState(current, N);
    if (index < 0) {
      if (nem->StartError(0)) {
        em->cerr() << "Out of memory when adding to state space";
        nem->DoneError();
      }
      return Out_Of_Memory; 
    }
    if (debug.startReport()) {
      debug.report() << "Valid state: ";
      debug.report().PutArray(current, N);
      debug.report() << "\n";
      debug.stopIO();
    }
    return Success;
  }
  for (current[k] = 0; current[k] < bounds[k]; current[k]++) {
    // Check for sigterm
    if (em->caughtTerm()) return terminateError(); 

    nem->GetVar(k)->SetToValueNumber(current[k]);
    if (nem->SatisfiesConstraintsAt(k)) {
      error e = Generate_NE_rec(k+1);
      if (e != Success) return e;
    }
  }
  return Success;
}

// **************************************************************************
// *                                                                        *
// *                         icp_ss_analyzer  class                         *
// *                                                                        *
// **************************************************************************

// abstract base class for min, max, sat engines
class icp_ss_analyzer : public subengine {
  static engtype* SSGen;
  friend void InitializeDCPEngines(exprman* em);
public:
  icp_ss_analyzer();
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual error SolveMeasure(hldsm* m, measure* what);
protected:
  virtual error SolveExplicit(no_event_model* nem, 
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

subengine::error icp_ss_analyzer::SolveMeasure(hldsm* hm, measure* what)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));

  result dummy;
#ifdef DEVELOPMENT_CODE
  dummy.setNull();
#endif
  error e = SSGen ? SSGen->runEngine(hm, dummy) : No_Engine;
  if (e) {
    return e;
  }
  if (0==hm->GetProcess()) {
    return Engine_Failed;
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

  return No_Engine;
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
  virtual error SolveExplicit(no_event_model* nem, 
    const StateLib::state_coll* sc, measure* what);
};

icp_minimize the_icp_minimize;

icp_minimize::icp_minimize() : icp_ss_analyzer()
{
}

subengine::error 
icp_minimize ::SolveExplicit(no_event_model* nem, 
  const StateLib::state_coll* sc, measure* what)
{
  DCASSERT(nem);
  DCASSERT(sc);
  DCASSERT(what);

  if (0==sc->Size()) {
    what->SetNull();
    return Success;
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
  return Success; 
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
  virtual error SolveExplicit(no_event_model* nem, 
    const StateLib::state_coll* sc, measure* what);
};

icp_maximize the_icp_maximize;

icp_maximize::icp_maximize() : icp_ss_analyzer()
{
}

subengine::error 
icp_maximize::SolveExplicit(no_event_model* nem, 
  const StateLib::state_coll* sc, measure* what)
{
  DCASSERT(nem);
  DCASSERT(sc);
  DCASSERT(what);
  
  if (0==sc->Size()) {
    what->SetNull();
    return Success;
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
  return Success; 
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
  virtual error SolveExplicit(no_event_model* nem, 
    const StateLib::state_coll* sc, measure* what);
};

icp_satisfiable the_icp_satisfiable;

icp_satisfiable::icp_satisfiable() : icp_ss_analyzer()
{
}

subengine::error 
icp_satisfiable::SolveExplicit(no_event_model* nem, 
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
  return Success; 
}

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeDCPEngines(exprman* em)
{
  if (0==em) return;

  // Initialize libraries
  static icp_state_lib state_lib_data;
  em->registerLibrary(&state_lib_data);

  // Initialize options
  option* report = em->findOption("Report");
  option* debug = em->findOption("Debug");

  icp_stategen::report.Initialize(report,
    "explicit_dcp_gen",
    "When set, explicit reachability set performance is reported.",
    false
  );

  icp_stategen::debug.Initialize(debug,
    "explicit_dcp_gen",
    "When set, explicit reachability set generation details are displayed.",
    false
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
}



