
// $Id$

#include "engine.h"
#include "../include/splay.h"
#include "../Options/options.h"
#include "measures.h"
#include "mod_inst.h"
#include "exprman.h"

#include <string.h>

// ********************************************************
// *                  engine_tree class                   *
// ********************************************************

class engine_tree : public SplayOfPointers <engine> {
public:
  engine_tree(int a, int b) : SplayOfPointers <engine> (a, b) { }
};

// ********************************************************
// *               engine_selection  class                *
// ********************************************************

class engine_selection : public radio_button {
  engine* solution;
  engtype* parent;
public:
  engine_selection(engtype* p, engine* s, int i);
  virtual ~engine_selection();
  virtual bool AssignToMe();
};

// ********************************************************
// *              engine_selection  methods               *
// ********************************************************

engine_selection::engine_selection(engtype* p, engine* s, int i)
 : radio_button(s->Name(), s->Documentation(), i)
{
  parent = p;
  solution = s;
  DCASSERT(solution->getType() == parent);
}

engine_selection::~engine_selection()
{
  delete solution;
  // don't delete parent!
}

bool engine_selection::AssignToMe()
{
  if (0==parent)  return false;
  parent->selected_engine = solution;
  return true;
}

// ********************************************************
// *                  subengine methods                   *
// ********************************************************

exprman* subengine::em = 0;

subengine::subengine()
{
}

subengine::~subengine()
{
}

const char* subengine::getNameOfError(error e)
{
  switch (e) {
    case Success:
        return "Success";

    case Finalized:
        return "Engine manager should / should not be finalized";

    case No_Engine:
        return "No solution engine available";

    case Bad_Option:
        return "Unknown engine selection";

    case Duplicate:
        return "Duplicate engine name";

    case Call_Mismatch:  
        return "Incorrect measure signature";

    case Out_Of_Memory:  
        return "Engine ran out of memory";

    case Terminated:  
        return "Engine was terminated";

    case Assertion_Failure:
        return "Assertion failure";

    case Engine_Failed:  
        return "Engine failed";
  }
  return "Unknown error";
}

subengine::error subengine::RunEngine(result*, int, traverse_data &)
{
  return Call_Mismatch;
}

subengine::error subengine::RunEngine(hldsm*, result &)
{
  return Call_Mismatch;
}

subengine::error subengine::SolveMeasure(hldsm* , measure* )
{
  return Call_Mismatch;
}

subengine::error subengine::SolveMeasures(hldsm* , set_of_measures* )
{
  return Call_Mismatch;
}


// ********************************************************
// *                   engine  methods                    *
// ********************************************************

exprman* engine::em = 0;
int engine::num_hlm_types = 0;

engine::engine(const char* n, const char* d)
{
  etype = 0;
  name = n;
  doc = d;
  next = 0;
  children = new subengine*[num_hlm_types];
  for (int i=0; i<num_hlm_types; i++) children[i] = 0;
}

engine::~engine()
{
  // Is this ever called?
  delete[] children;
}

void engine::AddSubEngine(subengine* child)
{
  if (0==child) return;
  for (int i=0; i<num_hlm_types; i++) {
    hldsm::model_type mi = (hldsm::model_type) i;
    if (child->AppliesToModelType(mi)) {
      if (children[i]) {
        if (em->startInternal(__FILE__, __LINE__)) {
          em->causedBy(0);
          em->internal() << "Registering subengine for " << name;
          em->internal() << " with existing model type " << i << "\n";
          em->stopIO();
        }
        DCASSERT(0);
      }
      children[i] = child;
    }
  } // for i
}

int engine::Compare(const char* name2) const
{
  if ( (0==name) && (0==name2) )  return 0;
  if (0==name)        return -1;
  if (0==name2)       return 1;
  return strcmp(name, name2);
}


// ******************************************************************
// *                        engtype  methods                        *
// ******************************************************************

engtype::engtype(const char* n, const char* d, calling_form f)
{
  name = n;
  doc = d;
  form = f;
  EngTree = 0;
  default_engine = 0;
  selected_engine = 0;
  index = 0;
}

engtype::~engtype()
{
  killEngTree();
}

subengine::error engtype::registerEngine(engine* e)
{
  if (0==e)  return subengine::No_Engine;
  e->etype = this;
  if (selected_engine)  return subengine::Finalized;
  if (Nothing == form) {
    if (0==default_engine) default_engine = e;
    return subengine::Success;
  }
  if (0==EngTree)  EngTree = new engine_tree(16, 0);
  engine* f = EngTree->Insert(e);
  if (f==e)  {
    if (0==default_engine) default_engine = e;
    return subengine::Success;
  }
  // there is an engine with the same name.
  delete e;
  return subengine::Duplicate;
}

subengine::error engtype::setDefaultEngine(engine* d)
{
  if (selected_engine) return subengine::Finalized;
  if (d) if (EngTree->FindIndex(d)<0) return subengine::No_Engine;
  default_engine = d;
  return subengine::Success;
}

subengine::error engtype::registerSubengine(const char* name, subengine* se)
{
  if (0==se)            return subengine::Success;
  if (0==name)          return subengine::No_Engine;
  if (selected_engine)  return subengine::Finalized;
  engine* f = EngTree->Find(name);
  if (0==f)             return subengine::No_Engine;
  f->AddSubEngine(se);
  return subengine::Success;
}

void engtype::finalizeRegistry(option_manager* om)
{
  if (selected_engine)  return;
  if (0==EngTree)  return;
  if (0==default_engine) {
    default_engine = EngTree->GetItem(0);
  }
  selected_engine = default_engine;
  selected_engine_index = EngTree->FindIndex(selected_engine);
  DCASSERT(selected_engine_index >= 0);
  long N = EngTree->NumElements();
  if (1 == N || 0==om) {
    // no need to build option
    killEngTree();
    return;
  } 
  engine** sorted_engines = new engine*[N];
  EngTree->CopyToArray(sorted_engines);
  radio_button** values = new radio_button* [N];
  for (int i=0; i<N; i++) {
    engine* e = sorted_engines[i];
    DCASSERT(e);
    DCASSERT(e->Name());
    DCASSERT(e->Documentation());
    values[i] = new engine_selection(this, e, i);
    if (selected_engine == e) selected_engine_index = i;
  } // for i
  // build the option
  option* foo = MakeRadioOption(Name(), Documentation(), 
        values, N, selected_engine_index);
  DCASSERT(om);
  om->AddOption(foo);
  delete[] sorted_engines;
  killEngTree();
}

subengine::error engtype::runEngine(result* pass, int np, traverse_data &x)
{
  if (0==selected_engine)  return subengine::No_Engine;
  return selected_engine->RunEngine(pass, np, x);
}

subengine::error engtype::runEngine(hldsm* m, result &p)
{
  if (0==selected_engine)  return subengine::No_Engine;
  return selected_engine->RunEngine(m, p);
}

subengine::error engtype::solveMeasure(hldsm* m, measure* what)
{
  if (0==selected_engine)  return subengine::No_Engine;
  return selected_engine->SolveMeasure(m, what);
}

subengine::error engtype::solveMeasures(hldsm* m, set_of_measures* list)
{
  if (0==selected_engine)  return subengine::No_Engine;
  return selected_engine->SolveMeasures(m, list);
}

set_of_measures* engtype::makeMeasureSet() const
{
  DCASSERT(Grouped != form);
  return 0;
}

void engtype::killEngTree()
{
  delete EngTree;
  EngTree = 0;
}

// ******************************************************************
// *                   unordered_engtype  methods                   *
// ******************************************************************

unordered_engtype::unordered_engtype(const char* n, const char* d)
 : engtype(n, d, Grouped)
{
}

set_of_measures* unordered_engtype::makeMeasureSet() const
{
  return MakeUnsortedMeasures();
}

// ******************************************************************
// *                      time_engtype methods                      *
// ******************************************************************

time_engtype::time_engtype(const char* n, const char* d)
 : engtype(n, d, Grouped)
{
}

set_of_measures* time_engtype::makeMeasureSet() const
{
  return MakeTimeSortedMeasures();
}


// ******************************************************************
// *                      func_engine  methods                      *
// ******************************************************************

func_engine::func_engine(const type* rt, const char* name, int np, engtype* w)
 : simple_internal(rt, name, np)
{
  whicheng = w;
  DCASSERT(em);
  engpass = new result[np];
  for (int i=0; i<np; i++) engpass[i].setNull();
}

func_engine::~func_engine()
{
  delete[] engpass;
}

void func_engine::Compute(traverse_data &x, expr** pass, int np)
{
  subengine::error e;
  if (whicheng) {
    e = BuildParams(x, pass, np);
    if (subengine::Success == e) e = whicheng->runEngine(engpass, np, x);
    for (int i=0; i<np; i++) engpass[i].setNull();
  } else {
    e = subengine::No_Engine;
  }

  if (subengine::Success == e)  return;

  switch (e) {
    case subengine::No_Engine:
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "No solution engine available for " << Name();
        formals.PrintHeader(em->cerr(), false);
        em->stopIO();
      };
      break;

    default:
      if (em->startInternal(__FILE__, __LINE__)) {
        em->causedBy(x.parent);
        em->internal() << "unanticipated error: ";
        em->internal() << subengine::getNameOfError(e);
        em->newLine();
        em->internal() << "for " << Name();
        formals.PrintHeader(em->internal(), false);
        em->internal() << " engine";
        em->stopIO();
      }
  }
  x.answer->setNull();
}


// ******************************************************************
// *                                                                *
// *                     redirect_engine  class                     *
// *                                                                *
// ******************************************************************

class redirect_engine : public subengine {
  engtype* link;
public:
  redirect_engine(engtype* l);

  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual error RunEngine(result* pass, int np, traverse_data &x);
  virtual error RunEngine(hldsm* m, result &p);
  virtual error SolveMeasure(hldsm* m, measure* what);
  virtual error SolveMeasures(hldsm* m, set_of_measures* list);
};

// ******************************************************************
// *                    redirect_engine  methods                    *
// ******************************************************************

redirect_engine::redirect_engine(engtype* l)
{
  link = l;
}

bool redirect_engine::AppliesToModelType(hldsm::model_type mt) const
{
  return true;
}

subengine::error 
redirect_engine::RunEngine(result* p, int np, traverse_data &x)
{
  DCASSERT(link);
  return link->runEngine(p, np, x);
}

subengine::error
redirect_engine::RunEngine(hldsm* m, result &p)
{
  DCASSERT(link);
  return link->runEngine(m, p);
}

subengine::error 
redirect_engine::SolveMeasure(hldsm* m, measure* what)
{
  DCASSERT(link);
  return link->solveMeasure(m, what);
}

subengine::error 
redirect_engine::SolveMeasures(hldsm* m, set_of_measures* list)
{
  DCASSERT(link);
  return link->solveMeasures(m, list);
}


// ******************************************************************
// *                                                                *
// *                       noop_engine  class                       *
// *                                                                *
// ******************************************************************

class noop_engine : public subengine {
public:
  virtual ~noop_engine();
  virtual error SolveMeasure(hldsm* m, measure* what);
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
};

// ******************************************************************
// *                      noop_engine  methods                      *
// ******************************************************************

noop_engine::~noop_engine()
{
}

subengine::error noop_engine::SolveMeasure(hldsm*, measure* what)
{
  if (0==what)  return Success;
  traverse_data x(traverse_data::Compute);
  result foo;
  x.answer = &foo;
  what->ComputeRHS(x);
  what->SetValue(foo);
  return Success;
}

bool noop_engine::AppliesToModelType(hldsm::model_type mt) const
{
  return (mt != 0);
}

noop_engine the_noop_engine;


// ******************************************************************
// *                                                                *
// *                       bogus_engine class                       *
// *                                                                *
// ******************************************************************

/** Used as a placeholder until the real engine is discovered.
*/
class bogus_engine : public subengine {
public:
  virtual ~bogus_engine();
  virtual error SolveMeasure(hldsm* m, measure* what);
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
};

// ******************************************************************
// *                      bogus_engine methods                      *
// ******************************************************************

bogus_engine::~bogus_engine()
{
}

subengine::error bogus_engine::SolveMeasure(hldsm*, measure* what)
{
  if (em->startInternal(__FILE__, __LINE__)) {
    em->causedBy(what);
    em->internal() << "Calling a placeholder engine.";
    em->stopIO();
  }
  DCASSERT(0);
  return No_Engine;  // Probably best, if we manage to return anything
}

bool bogus_engine::AppliesToModelType(hldsm::model_type mt) const
{
  return (mt != 0);
}

bogus_engine the_bogus_engine;

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

engine* MakeRedirectionEngine(const char* n, const char* d, engtype* et)
{
  engine* e = new engine(n, d);
  e->AddSubEngine(new redirect_engine(et));
  return e;
}


void InitEngines(exprman* em)
{
  DCASSERT(em);
  subengine::em = em;
  engine::em = em;
  engine::num_hlm_types = 1+hldsm::Last_Model_Type;

  // Set up special "no engine".

  em->NO_ENGINE = new engtype("No Engine", "No solution engine", engtype::Single);
  CHECK_RETURN( em->registerEngineType(em->NO_ENGINE), true );
  RegisterEngine(
    em->NO_ENGINE,
    "no-op",
    "Do nothing",
    &the_noop_engine
  );
  em->NO_ENGINE->finalizeRegistry(0);

  // Set up special "blocked" engine type.

  em->BLOCKED_ENGINE = new engtype("Blocked Engine", "Blocked measures", engtype::Single);
  CHECK_RETURN( em->registerEngineType(em->BLOCKED_ENGINE), true );
  RegisterEngine(
    em->BLOCKED_ENGINE,
    "fail",
    "Fail and bail out",
    &the_bogus_engine
  );
  em->BLOCKED_ENGINE->finalizeRegistry(0);
}

