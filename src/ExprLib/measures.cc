
// $Id$

#include "measures.h"
#include "mod_inst.h"
#include "mod_def.h"

#include "exprman.h"
#include "engine.h"

#include "strings.h"
#include "../include/heap.h"

// ******************************************************************
// *                                                                *
// *                        measure  methods                        *
// *                                                                *
// ******************************************************************

measure::measure(const expr* e, engtype* which, model_def* parent, expr* rhs)
  : symbol(e->Filename(), e->Linenumber(), e->Type(), 0)
{
  which_engine = which;
  SetSubstitution(false);
  owner = 0;
  value.setNull();
  class_deps = 0;
  solve_deps = 0;

  parent->AcceptMeasure(this);
  if (model_debug.startReport()) {
    model_debug.report() << "Building submeasure " << Name() << " for expr ";
    if (rhs) rhs->Print(model_debug.report(), 0);
    else     model_debug.report() << "null";
    model_debug.report() << "\n";
    model_debug.stopIO();
  }
  SetRHS(rhs);
  if (model_debug.startReport()) {
    model_debug.report() << "Done building submeasure " << Name() << "\n";
    model_debug.stopIO();
  }
}

measure::~measure()
{
  Delete(rhs);
  delete class_deps;
  delete solve_deps;
}

void measure::Solve(traverse_data &x)
{
  DCASSERT(owner);

  if (class_deps) {
    DCASSERT(em->BLOCKED_ENGINE == which_engine);
    int stop = class_deps->Length();
    for (int i=0; i<stop; i++) {
      if (0==class_deps) break;
      measure* m = smart_cast <measure*> (class_deps->Item(i));
      DCASSERT(m);
      m->Solve(x); 
    } // for i
  }

  DCASSERT(em->BLOCKED_ENGINE != which_engine);

  if (solve_deps) {
    DCASSERT(isBlocked());
    int stop = solve_deps->Length();
    for (int i=0; i<stop; i++) {
      if (0==solve_deps) break;
      measure* m = smart_cast <measure*> (solve_deps->Item(i));
      DCASSERT(m);
      m->Solve(x); 
    }
  }

  DCASSERT(!isBlocked());
  if (isComputed())   return;
  if (which_engine)   owner->SolveMeasure(x, this);
  else                x.answer->setNull();
}

void measure::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  Solve(x);
  if (isComputed())
    (*x.answer) = value;
  else
    x.answer->setNull();
}

void measure::Traverse(traverse_data &x)
{
  DCASSERT(0==x.aggregate);
  switch (x.which) {
    case traverse_data::GetMeasures:
        if (Type() != em->VOID) {
          if (x.elist)  x.elist->Append(this);
          if (x.slist)  x.slist->Append(this);
          DCASSERT(x.answer);
          x.answer->setInt(x.answer->getInt()+1);
        }
        return;

    default:
        symbol::Traverse(x);
  }
}

void measure::SetRHS(expr* r)
{
  rhs = r;
  setDefined();

  if (0==r || Type() == em->VOID) return;

  solve_deps = new List <symbol>;
  r->BuildSymbolList(traverse_data::GetMeasures, 0, solve_deps);
  waitDepList(solve_deps);
}

void measure::Affix()
{
  if (Type() == em->VOID) return;
  if (!isComputed()) setComputed();
  SetSubstitution(true);
  if (model_debug.startReport()) {
    model_debug.report() << "Measure " << Name() << " is computed\n";
    model_debug.stopIO();
  }
  notifyList();
}

void measure::notifyFrom(const symbol *p)
{
  symbol::notifyFrom(p);  // log it, if desired

  if (isComputed() || isReady()) return;

  if (class_deps) {
    DCASSERT(which_engine == em->BLOCKED_ENGINE);
    for (int i=0; i<class_deps->Length(); i++) {
      if (! class_deps->ReadItem(i)->isComputed()) return;
    } // for i
    // we're now done with classification dependencies!
    delete class_deps;
    class_deps = 0;
    classifyNow();
    DCASSERT(em->BLOCKED_ENGINE != which_engine);
    owner->GroupMeasure(this);
  } // if class_deps

  if (isComputed() || isReady()) return;

  if (solve_deps) {
    for (int i=0; i<solve_deps->Length(); i++) {
      if (! solve_deps->ReadItem(i)->isComputed()) return;
    } // for i
    // we're now done with solution dependencies!
    delete solve_deps;
    solve_deps = 0;
  }

  if (isComputed() || isReady()) return;

  setReady();
}

bool measure::isBlockedEngine() const
{
  return which_engine == em->BLOCKED_ENGINE;
}

void measure::classifyNow()
{
  if (which_engine != em->BLOCKED_ENGINE) return;
  if (em->startInternal(__FILE__, __LINE__)) {
    em->causedBy(this);
    em->internal() << "No classification method for blocked measure!";
    em->stopIO();
  }
}

void measure::waitDepList(List <symbol>* dl)
{
  if (0==dl) return;
  for (int i=0; i<dl->Length(); i++) 
    dl->Item(i)->addToWaitList(this);
}

// ******************************************************************
// *                                                                *
// *                      time_measure methods                      *
// *                                                                *
// ******************************************************************

time_measure
 ::time_measure(const expr* e, model_def* p, expr* rhs)
 : measure(e, em->BLOCKED_ENGINE, p, rhs)
{
  time = -1;
  stop_time = -1;
}

// ******************************************************************
// *                                                                *
// *                    set_of_measures  methods                    *
// *                                                                *
// ******************************************************************

set_of_measures::set_of_measures()
{
}

set_of_measures::~set_of_measures()
{
}

set_of_measures::msrnode* set_of_measures::free_list = 0;


// ******************************************************************
// *                                                                *
// *                    unordered_measures class                    *
// *                                                                *
// ******************************************************************

class unordered_measures : public set_of_measures {
  msrnode* ready_list;
  msrnode* blocked_list;
public:
  unordered_measures();
  virtual ~unordered_measures();
  
  virtual void addMeasure(measure* m);
  virtual measure* popMeasure();
  virtual bool hasAnyMeasures();
protected:
  void SearchForUnblocked();
};

// ******************************************************************
// *                   unordered_measures methods                   *
// ******************************************************************

unordered_measures::unordered_measures()
{
  ready_list = blocked_list = 0;
}

unordered_measures::~unordered_measures()
{
  while (ready_list) {
    msrnode* tmp = ready_list;
    ready_list = tmp->next;
    RecycleNode(tmp);
  }
  while (blocked_list) {
    msrnode* tmp = blocked_list;
    blocked_list = tmp->next;
    RecycleNode(tmp);
  }
}

void unordered_measures::addMeasure(measure* m)
{
  if (0==m)    return;
  if (m->isComputed())  return;
  msrnode* n = NewNode(m);
  if (m->isReady()) {
    n->next = ready_list;
    ready_list = n;
  } else {
    n->next = blocked_list;
    blocked_list = n;
  }
}

measure* unordered_measures::popMeasure()
{
  for (;;) {
    if (0==ready_list)  SearchForUnblocked();
    if (0==ready_list)  return 0;
    msrnode* tmp = ready_list;
    ready_list = ready_list->next;
    measure* m = tmp->msr;
    RecycleNode(tmp);
    DCASSERT(m->isReady());  // is it possible that m is already computed?
    return m;
  } // infinite loop
}

bool unordered_measures::hasAnyMeasures()
{
  return ready_list || blocked_list;
}

void unordered_measures::SearchForUnblocked()
{
  msrnode* prev = 0;
  msrnode* curr = blocked_list;
  while (curr) {
    DCASSERT(curr->msr);
    if (curr->msr->isBlocked()) {
      prev = curr;
      curr = curr->next;
      continue;
    }
    // remove from the blocked list
    msrnode* next = curr->next;
    if (prev)   prev->next = next;
    else        blocked_list = next;
    // add to the ready list
    curr->next = ready_list;
    ready_list = curr;
    // advance
    curr = next;
  } // while curr
}

// ******************************************************************
// *                                                                *
// *                      timeorder_msrs class                      *
// *                                                                *
// ******************************************************************

class timeorder_msrs : public set_of_measures {
  HeapOfPointers <time_measure> this_round;
  HeapOfPointers <time_measure> next_round;
  msrnode* blocked;
  double current;
public:
  timeorder_msrs();
  virtual ~timeorder_msrs();
  
  virtual void addMeasure(measure* m);
  virtual measure* popMeasure();
  virtual bool hasAnyMeasures();
protected:
  void Blocked2Next();
};

// ******************************************************************
// *                     timeorder_msrs methods                     *
// ******************************************************************

timeorder_msrs::timeorder_msrs()
{
  blocked = 0;
  current = 0.0;
}

timeorder_msrs::~timeorder_msrs()
{
}

void timeorder_msrs::addMeasure(measure* m)
{
  time_measure* tm = smart_cast <time_measure*> (m);
  DCASSERT(tm);
  if (tm->GetTime() < current)  next_round.Insert(tm);
  else                          this_round.Insert(tm);
}

measure* timeorder_msrs::popMeasure()
{
  msrnode* blocked_new = 0;
  while (blocked) {
    msrnode* tmp = blocked;
    blocked = blocked->next;
    if (tmp->msr->isBlocked()) {
      tmp->next = blocked_new;
      blocked_new = tmp;
    } else {
      addMeasure(tmp->msr);
      RecycleNode(tmp);
    }
  } // while blocked;
  blocked = blocked_new;

  for (;;) {
    // if current round is empty, then we are done
    if (this_round.Length()==0) {
      Blocked2Next();
      current = 0.0;
      SWAP(this_round, next_round);  
      return 0;
    }
    time_measure* tm;
    this_round.Remove(tm);
    DCASSERT(tm);
    if (tm->GetTime() > current) Blocked2Next();
    current = tm->GetTime();
    if (tm->isBlocked()) {
      msrnode* tmp = NewNode(tm);
      tmp->next = blocked;
      blocked = tmp->next;
    } else {
      return tm;
    }
  } // infinite loop
}

bool timeorder_msrs::hasAnyMeasures()
{
  return (blocked || this_round.Length() > 0 || next_round.Length() > 0);
}

void timeorder_msrs::Blocked2Next()
{
  while (blocked) {
    msrnode* tmp = blocked;
    blocked = blocked->next;
    time_measure* tm = smart_cast <time_measure*> (tmp->msr);
    DCASSERT(tm);
    RecycleNode(tmp);
    next_round.Insert(tm);
  } // while blocked;
}

// ******************************************************************
// *                                                                *
// *                        msr_func methods                        *
// *                                                                *
// ******************************************************************

msr_func::msr_func(eng_class ec, const type* t, const char* name, int nf)
 : simple_internal(t, name, nf)
{
  ect = ec;
}

msr_func::~msr_func()
{
}

void msr_func::Compute(traverse_data &x, expr** pass, int np)
{
  if (em->startInternal(__FILE__, __LINE__)) {
    em->noCause();
    em->internal() << "Trying to compute measure-generating function: ";
    Print(em->internal(), 0);
    em->stopIO();
  }
}

int msr_func::Traverse(traverse_data &x, expr** pass, int np)
{
  if (traverse_data::Substitute != x.which) {
    return simple_internal::Traverse(x, pass, np);
  }

  measure* subst = buildMeasure(x, pass, np);
  if (0==subst)   return 0;
  if (em->BLOCKED_ENGINE != subst->EngineType()) {
    DCASSERT(x.model);
    x.model->GroupMeasure(subst);
  } else {
    subst->notifyFrom(0);
  }

  x.answer->setPtr(subst);
  return 1;
}

// ******************************************************************
// *                                                                *
// *                      msr_noengine methods                      *
// *                                                                *
// ******************************************************************

msr_noengine
::msr_noengine(eng_class ec, const type* t, const char* name, int nf)
 : msr_func(ec, t, name, nf)
{
}

msr_noengine::~msr_noengine()
{
}

void msr_noengine::Compute(traverse_data &x, expr** pass, int np)
{
  if (em->startInternal(__FILE__, __LINE__)) {
    em->noCause();
    em->internal() << "No Compute() method provided for function: ";
    Print(em->internal(), 0);
    em->stopIO();
  }
}

measure* msr_noengine
::buildMeasure(traverse_data &x, expr** pass, int np)
{
  const char* fn = x.parent ? x.parent->Filename() : 0;
  int ln = x.parent ? x.parent->Linenumber() : -1;
  expr* comp = em->makeFunctionCall(fn, ln, this, pass, np);
  engtype* et = em->NO_ENGINE;
  return new measure(x.parent, et, x.model, comp);
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

set_of_measures* MakeUnsortedMeasures()
{
  return new unordered_measures;
}

/// Build a collection of measures, sorted by "time".
set_of_measures* MakeTimeSortedMeasures()
{
  return new timeorder_msrs;
}


