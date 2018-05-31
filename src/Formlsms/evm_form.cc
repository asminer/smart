
#include "evm_form.h"
#include "../Options/options.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/formalism.h"
#include "../ExprLib/casting.h"

#include "../ExprLib/sets.h"
#include "../ExprLib/mod_def.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/dd_front.h"
#include "dsde_hlm.h"

#include "../include/splay.h"


// **************************************************************************
// *                                                                        *
// *                           assign_entry class                           *
// *                                                                        *
// **************************************************************************

/// Each transition maintains a collection of these.
class assign_entry {
  model_var* lhs;
  expr* rhs;
  // Built during "compilation"
  expr* firing;
public:
  assign_entry();
  ~assign_entry();

  inline void setLHS(model_var* p) {
    DCASSERT(0==rhs);
    Delete(lhs);
    lhs = Share(p);
  }
  inline void setRHS(expr* x) {
    DCASSERT(0==rhs);
    rhs = Share(x);
  }
  inline expr* getFiring() {
    return firing;
  }

  inline int Compare(const model_var* p) const { 
    return SIGN(SafeID(lhs) - SafeID(p));
  }
  inline int Compare(const assign_entry* x) const {
    DCASSERT(x);
    return Compare(x->lhs);
  }

  void Compile(const exprman* em);
};

// **************************************************************************
// *                          assign_entry methods                          *
// **************************************************************************

assign_entry::assign_entry()
{
  lhs = 0;
  rhs = 0;
  firing = 0;
}

assign_entry::~assign_entry()
{
  Delete(lhs);
  Delete(rhs);
}

void assign_entry::Compile(const exprman* em)
{
  DCASSERT(0==firing);
  firing = MakeVarAssign(em, lhs, rhs);
}

// **************************************************************************
// *                                                                        *
// *                            evm_intvar class                            *
// *                                                                        *
// **************************************************************************

class evm_intvar : public model_statevar {
  long init;
  bool has_init;
public:
  evm_intvar(const symbol* w, const model_instance* p, shared_object* b);

  inline bool hasInit() const { return has_init; }
  inline void setInit(long i) {
    DCASSERT(!has_init);
    init = i;
    has_init = true;
  }
  inline long getInit() const { 
    DCASSERT(has_init);
    return init;
  }

  virtual void ComputeInState(traverse_data &x) const;
  virtual void AddToState(traverse_data &x, long delta) const;
  virtual void SetNextState(traverse_data &x, shared_state* ns, long newst) const;

  long State2Long(const shared_state* x) const;
  void SetInit(shared_state* x) const;
};

// ******************************************************************
// *                       evm_intvar methods                       *
// ******************************************************************

evm_intvar
::evm_intvar(const symbol* w, const model_instance* p, shared_object* b)
 : model_statevar(w, p, b)
{
  has_init = false;
}

void evm_intvar::ComputeInState(traverse_data &x) const
{
  DCASSERT(x.current_state);
  const hldsm* hm = x.current_state->Parent();
  DCASSERT(hm);
  if (hm->GetParent() != getParent()) {
    ownerError(x);
  } else {
    long index = x.current_state->get(GetIndex());
    DCASSERT(bounds);
    DCASSERT(x.answer);
    bounds->GetElement(index, *x.answer);
  }
}

void evm_intvar::AddToState(traverse_data &x, long delta) const
{
  DCASSERT(x.answer);
  DCASSERT(x.current_state);
  DCASSERT(x.next_state);
  DCASSERT(bounds);
  
  result foo;
  long index = x.current_state->get(GetIndex());
  bounds->GetElement(index, foo);
  if (foo.isNormal()) {
    foo.setInt(foo.getInt() + delta);
    index = bounds->IndexOf(foo);
    if (index < 0)  boundsError(x, foo.getInt());
  } 
  x.next_state->set(GetIndex(), index);
}

void evm_intvar
::SetNextState(traverse_data &x, shared_state* ns, long newst) const
{
  DCASSERT(x.answer);
  DCASSERT(bounds);
  
  result foo;
  foo.setInt(newst);
  long index = bounds->IndexOf(foo);
  if (index < 0) boundsError(x, newst);

  DCASSERT(ns);
  ns->set(GetIndex(), index);
}

long evm_intvar::State2Long(const shared_state* x) const
{
  DCASSERT(bounds);

  result foo;
  long index = x->get(GetIndex());
  bounds->GetElement(index, foo);
  DCASSERT(foo.isNormal());
  return foo.getInt();
}

void evm_intvar::SetInit(shared_state* x) const
{
  DCASSERT(bounds);
  if (!has_init) {
    x->set(GetIndex(), 0);
    return;
  }
  result foo;
  foo.setInt(init);
  long index = bounds->IndexOf(foo);
  DCASSERT(index >= 0);
  x->set(GetIndex(), index);
}

// **************************************************************************
// *                                                                        *
// *                            evm_event  class                            *
// *                                                                        *
// **************************************************************************

/** Used to store information during model construction.
    Once the model has been finalized, most of this info
    goes away.
*/
class evm_event : public model_event {
  /// Data used during model construction
  struct extra_info {
    /// variables set by this transition
    SplayOfPointers <assign_entry> *modlist;
    /// guard expressions
    List <expr>* guards;

  public:
    extra_info();
    ~extra_info();
  };
  /// 0 after model is finalized.
  extra_info* build_data;
public:
  evm_event(const symbol* wrapper, const model_instance* p);

  void addEnabling(expr* guard);

  /// Returns true if v already had assignment in this event.
  bool setAssignment(assign_entry* &tmp, expr* rhs);

  void Finalize(OutputStream &ds);
};

// **************************************************************************
// *                           evm_event  methods                           *
// **************************************************************************

evm_event::extra_info::extra_info()
{
  modlist = 0;
  guards = 0;
}

evm_event::extra_info::~extra_info()
{
  if (modlist) modlist->DeleteAndClear();
  delete modlist;
  delete guards;
}

evm_event::evm_event(const symbol* wrapper, const model_instance* p)
 : model_event(wrapper, p)
{
  build_data = new extra_info;
}

void evm_event::addEnabling(expr* guard)
{
  DCASSERT(guard);
  DCASSERT(build_data);
  // bool answer = true;
  if (0==build_data->guards) {
    build_data->guards = new List <expr>;
    // answer = false;
  }
  build_data->guards->Append(guard);
}

bool evm_event::setAssignment(assign_entry* &tmp, expr* rhs)
{
  DCASSERT(tmp);
  DCASSERT(build_data);
  if (0==build_data->modlist)
    build_data->modlist = new SplayOfPointers <assign_entry> (16, 0);
  assign_entry* find = build_data->modlist->Insert(tmp);
  if (find != tmp) {
    tmp->setLHS(0);
    return true;
  }
  tmp = 0;
  DCASSERT(find);
  find->setRHS(rhs);
  return false;
}

void evm_event::Finalize(OutputStream &ds)
{
  // First, traverse "guards" to build enabling expression
  int ng;
  expr** guards;
  if (build_data->guards) {
    ng = build_data->guards->Length();
    guards = build_data->guards->CopyAndClear();
  } else {
    ng = 0;
    guards = 0;
  }
  if (ng > 1) {
    setEnabling(em->makeAssocOp(0, -1, exprman::aop_and, guards, 0, ng));
  } else {
    if (1==ng) {
      setEnabling(guards[0]);
    } else {
      result always_enabled;
      always_enabled.setBool(true);
      setEnabling(em->makeLiteral(0, -1, em->BOOL, always_enabled));
    }
    delete[] guards;
  }

  // Traverse assignment list to build overall next-state
  int na = (build_data->modlist) ? build_data->modlist->NumElements() : 0;
  expr** nextlist = na ? new expr*[na] : 0;
  for (int i=0; i<na; i++) {
    assign_entry* a = build_data->modlist->GetItem(i);
    DCASSERT(a);
    a->Compile(em);
    nextlist[i] = a->getFiring();
  }
  if (na > 1) {
    setNextstate(em->makeAssocOp(0, -1, exprman::aop_semi, nextlist, 0, na));
  } else {
    if (1==na) setNextstate(nextlist[0]);
    delete[] nextlist;
  }
  
  delete build_data;
  build_data = 0;
}

// **************************************************************************
// *                                                                        *
// *                             evm_hlm  class                             *
// *                                                                        *
// **************************************************************************

class evm_hlm : public dsde_hlm {
protected:
  static int StateStyle;
  static const int INDEXED = 0;
  static const int SAFE    = 1;
  static const int SPARSE  = 2;
  static const int VECTOR  = 3;

  friend class init_evmform;
public:
  evm_hlm(const model_instance* s, model_statevar** V, int nv, model_event** E, int ne);
  virtual ~evm_hlm();

  // required for hldsm:
  virtual void showState(OutputStream &s, const shared_state* x) const;

  // required for dsde_hlm:
  virtual int NumInitialStates() const;
  virtual double GetInitialState(int n, shared_state* s) const;
};

int evm_hlm::StateStyle;

// ******************************************************************
// *                        evm_hlm  methods                        *
// ******************************************************************

evm_hlm::evm_hlm(const model_instance* s, model_statevar** V, int nv, model_event** E, int ne)
 : dsde_hlm(s, V, nv, E, ne, 0, 0)
{
}

evm_hlm::~evm_hlm()
{
}

void evm_hlm::showState(OutputStream &s, const shared_state* st) const
{
  DCASSERT(st);

  bool printed = false;
  int i;
  s << '[';
  for (i=0; i<num_vars; i++) {
    evm_intvar* sv = smart_cast <evm_intvar*> (state_data[i]);
    DCASSERT(sv);
    long tk = sv->State2Long(st);

    if (0==tk && (SPARSE == StateStyle || SAFE == StateStyle))
      continue;

    if (printed) s << ", ";
    printed = true;

    switch (StateStyle) {
      case VECTOR:
          s << tk;
          continue;

      case SAFE:
          if (1==tk) {
            s << state_data[i]->Name();
            continue;
          }
          // no continue

      case INDEXED:
      case SPARSE:
          s << state_data[i]->Name() << ":" << tk;
          continue;

      default:
          if (em->startInternal(__FILE__, __LINE__)) {
            em->noCause();
            em->internal() << "Unknown state style " << StateStyle;
            em->stopIO();
          }
    } // switch
  } // for i

  s << ']';
}

int evm_hlm::NumInitialStates() const
{
  return 1;
}

double evm_hlm::GetInitialState(int n, shared_state* st) const
{
  DCASSERT(0==n);
  DCASSERT(st);
  DCASSERT(state_data);
  for (int i=0; i<num_vars; i++) {
    evm_intvar* iv = smart_cast <evm_intvar*> (state_data[i]);
    DCASSERT(iv);
    iv->SetInit(st);
  }
  return 1.0;
}


// **************************************************************************
// *                                                                        *
// *                             evm_def  class                             *
// *                                                                        *
// **************************************************************************

/// Smart support for the EVM formalism.
class evm_def : public dsde_def {
  bool error;

  symbol* varlist;
  int num_vars;
  symbol* eventlist;
  int num_events;

  List <expr> *assertion_list;

  static assign_entry* tmp_arc;

  static named_msg evm_debug;
  static named_msg dup_range;
  static named_msg dup_assign;
  static named_msg dup_init;
  static named_msg dup_hide;
  static named_msg dup_part;
  static named_msg no_event;
  static named_msg no_vars;
  static named_msg no_part;

  static const type* intvar_type;
  static const type* event_type;

  friend class init_evmform;
public:
  evm_def(const char* fn, int line, const type* t, char*n, 
      formal_param **pl, int np);

  virtual ~evm_def();

  // Required for models:
  virtual model_var* MakeModelVar(const symbol* wrap, shared_object* bnds);

  // For model construction:
  void AddRange(const expr* call, shared_set* vset, shared_set* range);
  void AddEnabling(const expr* call, evm_event* t, expr* guard);
  void AddAssignment(const expr* call, model_var* v, evm_event* t, expr* rhs);
  void AddInit(const expr* call, model_var* v, long rhs);
  void HideEvent(const expr* call, evm_event* t);
  void AddAssertion(expr* a);

protected:
  virtual void InitModel();
  virtual void FinalizeModel(OutputStream &ds);
};

assign_entry* evm_def::tmp_arc = 0;
named_msg evm_def::evm_debug;
named_msg evm_def::dup_range;
named_msg evm_def::dup_assign;
named_msg evm_def::dup_init;
named_msg evm_def::dup_hide;
named_msg evm_def::dup_part;
named_msg evm_def::no_event;
named_msg evm_def::no_vars;
named_msg evm_def::no_part;

const type* evm_def::intvar_type;
const type* evm_def::event_type;

// ******************************************************************
// *                        evm_def  methods                        *
// ******************************************************************

evm_def::evm_def(const char* fn, int line, const type* t, 
   char*n, formal_param **pl, int np) : dsde_def(fn, line, t, n, pl, np)
{
  error = 0;
}

evm_def::~evm_def()
{
}

model_var* evm_def::MakeModelVar(const symbol* wrap, shared_object* bnds)
{
  if (error) return 0;
  DCASSERT(wrap);
  DCASSERT(0==bnds);

  if (evm_debug.startReport()) {
    evm_debug.report() << "adding ";
    evm_debug.report() << wrap->Type()->getName();
    evm_debug.report() << " " << wrap->Name() << "\n";
    evm_debug.stopIO();
  }

  if (wrap->Type() == intvar_type) {
    model_statevar* v = new evm_intvar(wrap, current, bnds);
    v->SetIndex(num_vars);
    num_vars++;
    v->LinkTo(varlist);
    varlist = v;
    return v;
  }

  if (wrap->Type() == event_type) {
    DCASSERT(0==bnds);
    model_var* v = new evm_event(wrap, current);
    num_events++;
    v->LinkTo(eventlist);
    eventlist = v;
    return v;
  }

  DCASSERT(0);
  return 0;
}

void evm_def::AddRange(const expr* call, shared_set* vset, shared_set* range)
{
  DCASSERT(vset);

  if (0==range) return;

  result elem;
  for (int z=0; z<vset->Size(); z++) {
    vset->GetElement(z, elem);
    DCASSERT(elem.isNormal());
    model_statevar* pl = smart_cast <model_statevar*> (elem.getPtr());
    DCASSERT(pl);  
    if (!isVariableOurs(pl, call, "ignoring range")) continue;

    if (pl->HasBounds()) {
      if (StartWarning(dup_range, call)) {
        em->warn() << "Duplicate range for variable " << pl->Name();
        em->warn() << ", ignoring";
        DoneWarning();
      }
      continue;
    }

    if (evm_debug.startReport()) {
      evm_debug.report() << "setting range ";
      range->Print(evm_debug.report(), 0);
      evm_debug.report() << " for variable ";
      evm_debug.report() << pl->Name() << "\n";
      evm_debug.stopIO();
    }

    pl->SetBounds(Share(range));
  } // for z

}

void evm_def::AddEnabling(const expr* call, evm_event* t, expr* guard)
{
  DCASSERT(t);
  DCASSERT(guard);
  if (!isVariableOurs(t, call, "ignoring enabling")) return;

  if (evm_debug.startReport()) {
    evm_debug.report() << "adding enabling condition for ";
    evm_debug.report() << t->Name() << ": ";
    guard->Print(evm_debug.report(), 0);
    evm_debug.report().Put('\n');
    evm_debug.stopIO();
  }

  t->addEnabling(guard);
}


void evm_def
::AddAssignment(const expr* call, model_var* v, evm_event* t, expr* rhs)
{
  DCASSERT(v);
  DCASSERT(t);
  DCASSERT(rhs);

  if (!isVariableOurs(v, call, "ignoring assignment")) return;
  if (!isVariableOurs(t, call, "ignoring assignment")) return;

  if (evm_debug.startReport()) {
    evm_debug.report() << "adding assignment for event ";
    evm_debug.report() << t->Name() << ":   ";
    evm_debug.report() << v->Name() << " <- ";
    rhs->Print(evm_debug.report(), 0);
    evm_debug.report().Put('\n');
    evm_debug.stopIO();
  }

  if (0==tmp_arc) tmp_arc = new assign_entry;
  tmp_arc->setLHS(v);
  if (t->setAssignment(tmp_arc, rhs)) {
      if (StartWarning(dup_assign, call)) {
        em->warn() << "Duplicate assignment for " << v->Name();
        em->warn() << " in event ";
        em->warn() << t->Name() << ", ignoring";
        DoneWarning();
      }
  }
}

void evm_def::AddInit(const expr* call, model_var* v, long initval)
{
  DCASSERT(v);
  if (!isVariableOurs(v, call, "ignoring init")) return;

  if (evm_debug.startReport()) {
    evm_debug.report() << "setting initial value for ";
    evm_debug.report() << v->Name() << " : " << initval << "\n";
    evm_debug.stopIO();
  }

  evm_intvar* iv = smart_cast <evm_intvar*> (v);
  DCASSERT(iv);
  
  if (iv->hasInit()) {
    if (StartWarning(dup_init, call)) {
        em->warn() << "Duplicate initial value for " << v->Name();
        em->warn() << ", ignoring";
        DoneWarning();
    }
    return;
  }

  iv->setInit(initval);
}

void evm_def::HideEvent(const expr* call, evm_event* t)
{
  DCASSERT(t);

  if (!isVariableOurs(t, call, "ignoring hide")) return;

  if (evm_debug.startReport()) {
    evm_debug.report() << "hiding event " << t->Name() << "\n";
    evm_debug.stopIO();
  }

  if (!t->hasFiringType(model_event::Unknown)) {
    if (StartWarning(dup_hide, call)) {
      em->warn() << "Event " << t->Name() << " already designated, ignoring hide";
      DoneWarning();
    }
    return;
  }

  t->setHidden();
}

void evm_def::AddAssertion(expr* a)
{
  DCASSERT(a);
  DCASSERT(assertion_list);
  expr* mya = a->Substitute(0);
  mya->PreCompute();

  if (evm_debug.startReport()) {
    evm_debug.report() << "adding assertion ";
    mya->Print(evm_debug.report(), 0);
    evm_debug.report().Put('\n');
    evm_debug.stopIO();
  }

  assertion_list->Append(mya);
}

void evm_def::InitModel()
{
  dsde_def::InitModel();
  error = false;
  num_vars = num_events = 0;
  varlist = eventlist = 0;
  assertion_list = new List <expr>;
}

void evm_def::FinalizeModel(OutputStream &ds)
{
  // Put state vars into an array
  model_statevar** svs;
  if (0==num_vars) {
    svs = 0;
    if (StartWarning(no_vars, 0)) {
      em->warn() << "No variables defined";
      DoneWarning();
    }
  }  else {
    svs = new model_statevar*[num_vars];
    for (int i=num_vars-1; i>=0; i--) {
      evm_intvar* iv = smart_cast <evm_intvar*> (varlist);
      DCASSERT(iv);
      if (iv->hasInit()) {
        // check that initial value is within bounds
        result foo;
        foo.setInt(iv->getInit());
        if (iv->GetValueIndex(foo)<0) {
          // error here
        }
      }
      svs[i] = iv;
      varlist = varlist->Next();
      svs[i]->LinkTo(0);
    } // for i
  }
  DCASSERT(0==varlist);

  PartitionVars(svs, num_vars);

  // Put the events into an array and "compile" them
  model_event** elist;
  if (0==num_events) {
    elist = 0;
    if (StartWarning(no_event, 0)) {
      em->warn() << "No events defined";
      DoneWarning();
    }
  } else {
    elist = new model_event*[num_events];
    for (int i=num_events-1; i>=0; i--) {
      evm_event* t = smart_cast <evm_event*> (eventlist);
      DCASSERT(t);
      t->Finalize(ds);
      if (t->hasFiringType(model_event::Unknown)) {
        t->setNondeterministic();
      }
      elist[i] = t;
      eventlist = eventlist->Next();
      elist[i]->LinkTo(0);
    } // for i
  }

  evm_hlm* build = new evm_hlm(current, svs, num_vars, elist, num_events);

  // add assertions here...

  ConstructionSuccess(build);
}

// **************************************************************************
// *                                                                        *
// *                          evm_formalism  class                          *
// *                                                                        *
// **************************************************************************

class evm_formalism : public formalism {
public:
  evm_formalism(const char* n, const char* sd, const char* ld);

  virtual model_def* makeNewModel(const char* fn, int ln, char* name,
          symbol** formals, int np) const;

  virtual bool canDeclareType(const type* vartype) const;
  virtual bool canAssignType(const type* vartype) const;
  virtual bool includeCTL() const { return true; }
};

// ******************************************************************
// *                     evm_formalism  methods                     *
// ******************************************************************


evm_formalism
::evm_formalism(const char* n, const char* sd, const char* ld)
 : formalism(n, sd, ld)
{
}

model_def* evm_formalism::makeNewModel(const char* fn, int ln, char* name, 
          symbol** formals, int np) const
{
  return new evm_def(fn, ln, this, name, (formal_param**) formals, np);
}

bool evm_formalism::canDeclareType(const type* vartype) const
{
  if (0==vartype)                 return 0;
  if (vartype->matches("intvar")) return 1;
  if (vartype->matches("event"))  return 1;
  return 0;
}

bool evm_formalism::canAssignType(const type* vartype) const
{
  return 0;
}

// **************************************************************************
// *                                                                        *
// *                             EVM  Functions                             *
// *                                                                        *
// **************************************************************************

// ********************************************************
// *                    evm_eval class                    *
// ********************************************************

class evm_eval : public model_internal {
public:
  evm_eval();
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

evm_eval::evm_eval() : model_internal(em->INT->addProc(), "eval", 2)
{
  const type* intvar = em->findType("intvar");
  SetFormal(1, intvar, "v");
  SetDocumentation("The value of variable v in the current state.");
}

void evm_eval::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(x.current_state);
  DCASSERT(np==2);
  // Get place from second parameter
  SafeCompute(pass[1], x);
  if (!x.answer->isNormal())  return;

  DCASSERT(x.answer->isNormal());
  model_var* place = smart_cast <model_var*> (x.answer->getPtr());
  DCASSERT(place);

  place->ComputeInState(x);
}

int evm_eval::Traverse(traverse_data &x, expr** pass, int np)
{
  if (x.which != traverse_data::BuildDD) {
    return model_internal::Traverse(x, pass, np);
  }
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(np==2);
  DCASSERT(x.ddlib);

  // Get place from second parameter
  SafeCompute(pass[1], x);
  if (!x.answer->isNormal())  return 0;

  DCASSERT(x.answer->isNormal());
  symbol* pl = smart_cast <symbol*> (x.answer->getPtr());
  DCASSERT(pl);

  shared_object* pldd = x.ddlib->makeEdge(0);
  DCASSERT(pldd);
  x.ddlib->buildSymbolicSV(pl, false, 0, pldd);
  x.answer->setPtr(pldd);
  return 0;
}


// ********************************************************
// *                   evm_range  class                   *
// ********************************************************

class evm_range : public model_internal {
public:
  evm_range();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

evm_range::evm_range() : model_internal(em->VOID, "range", 2)
{
  typelist* t = new typelist(2);
  const type* place = em->findType("intvar");
  DCASSERT(place);
  t->SetItem(0, place->getSetOfThis());
  t->SetItem(1, em->INT->getSetOfThis());
  SetFormal(1, t, "vset:r");
  SetRepeat(1);
  SetDocumentation("For each variable v in set vset, set the range of values of v to r.");
}

void evm_range::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  evm_def* mdl = smart_cast<evm_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;

  for (int i=1; i<np; i++) {
    DCASSERT(pass[i]);

    result second;
    x.answer = &second;
    x.aggregate = 1;
    SafeCompute(pass[i], x);
    if (! second.isNormal() ) {
      mdl->StartError(pass[i]);
      em->cerr() << "Bad integer set for range, ignoring";
      mdl->DoneError();
      continue;
    }
    shared_set* rs = smart_cast<shared_set*>(second.getPtr());
    DCASSERT(rs);

    result first;
    x.answer = &first;
    x.aggregate = 0;
    SafeCompute(pass[i], x);
    DCASSERT(first.isNormal());
    shared_set* ps = smart_cast <shared_set*> (first.getPtr());
    mdl->AddRange(pass[i], ps, rs);
  }
  x.answer = answer;
  x.aggregate = 0;
}


// ********************************************************
// *                  evm_enabled  class                  *
// ********************************************************

class evm_enabled : public model_internal {
public:
  evm_enabled();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

evm_enabled::evm_enabled() : model_internal(em->VOID, "guard", 2)
{
  typelist* t = new typelist(2);
  const type* trans = em->findType("event");
  DCASSERT(trans);
  t->SetItem(0, trans->getSetOfThis());
  t->SetItem(1, em->BOOL->addProc());
  SetFormal(1, t, "vset:c");
  SetRepeat(1);
  SetDocumentation("For each variable v in set vset, add c as a guard for event v (i.e., v cannot occur if c is false).");
}

void evm_enabled::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  evm_def* mdl = smart_cast<evm_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;

  result first;
  x.answer = &first;
  for (int i=1; i<np; i++) {
    DCASSERT(pass[i]);
    SafeCompute(pass[i], x);
    DCASSERT(first.isNormal());
    shared_set* es = smart_cast <shared_set*> (first.getPtr());
    DCASSERT(es);
    expr* guard = pass[i]->Substitute(1);

    for (int z=0; z<es->Size(); z++) {
      result elem;
      es->GetElement(z, elem);
      evm_event* e = smart_cast <evm_event*> (elem.getPtr());
      DCASSERT(e);
      mdl->AddEnabling(pass[i], e, Share(guard));
    } // for z

    Delete(guard);
  }
  x.answer = answer;
  x.aggregate = 0;
}

// ********************************************************
// *                   evm_assign class                   *
// ********************************************************

class evm_assign : public model_internal {
public:
  evm_assign();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

evm_assign::evm_assign() : model_internal(em->VOID, "assign", 2)
{
  typelist* t = new typelist(3);
  const type* place = em->findType("intvar");
  const type* trans = em->findType("event");
  DCASSERT(place);
  t->SetItem(0, place);
  t->SetItem(1, trans);
  t->SetItem(2, em->INT->addProc());
  SetFormal(1, t, "v:e:n");
  SetRepeat(1);
  SetDocumentation("When event e occurs, set the value of v to be n.");
}

void evm_assign::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  evm_def* mdl = smart_cast<evm_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;

  for (int i=1; i<np; i++) {
    DCASSERT(pass[i]);

    result first;
    x.answer = &first;
    x.aggregate = 0;
    SafeCompute(pass[i], x);
    DCASSERT(first.isNormal());
    model_var* v = smart_cast <model_var*> (first.getPtr());
    DCASSERT(v);

    result second;
    x.answer = &second;
    x.aggregate = 1;
    SafeCompute(pass[i], x);
    DCASSERT(second.isNormal());
    evm_event* e = smart_cast <evm_event*> (second.getPtr());
    DCASSERT(e);

    mdl->AddAssignment(pass[i], v, e, pass[i]->Substitute(2));
  }
  x.answer = answer;
  x.aggregate = 0;
}


// ********************************************************
// *                   evm_init  class                   *
// ********************************************************

class evm_init : public model_internal {
public:
  evm_init();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

evm_init::evm_init() : model_internal(em->VOID, "init", 2)
{
  typelist* t = new typelist(2);
  const type* place = em->findType("intvar");
  DCASSERT(place);
  t->SetItem(0, place->getSetOfThis());
  t->SetItem(1, em->INT);
  SetFormal(1, t, "vset:n");
  SetRepeat(1);
  SetDocumentation("For each variable v in set vset, set the initial value of v to n.  If this is not specified for some variable v, the default is the first value in the range set.");
}

void evm_init::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  evm_def* mdl = smart_cast<evm_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;

  for (int i=1; i<np; i++) {
    DCASSERT(pass[i]);

    result second;
    x.answer = &second;
    x.aggregate = 1;
    SafeCompute(pass[i], x);
    if (! second.isNormal() ) {
      mdl->StartError(pass[i]);
      em->cerr() << "Bad integer for init, ignoring";
      mdl->DoneError();
      continue;
    } 

    result first;
    x.answer = &first;
    x.aggregate = 0;
    SafeCompute(pass[i], x);
    DCASSERT(first.isNormal());
    shared_set* ps = smart_cast <shared_set*> (first.getPtr());
    for (int z=0; z<ps->Size(); z++) {
      result elem;
      ps->GetElement(z, elem);
      model_var* v = smart_cast <model_var*> (elem.getPtr());
      DCASSERT(v);
      mdl->AddInit(pass[i], v, second.getInt());
    } // for z
  }
  x.answer = answer;
  x.aggregate = 0;
}

// ********************************************************
// *                    evm_hide class                    *
// ********************************************************

class evm_hide : public model_internal {
public:
  evm_hide();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

evm_hide::evm_hide() : model_internal(em->VOID, "hide", 2)
{
  const type* trans = em->findType("event");
  DCASSERT(trans);
  SetFormal(1, trans, "e");
  SetRepeat(1);
  SetDocumentation("Hide event e.  Hidden events are similar to immediate ones, but are untimed and non-deterministic.");
}

void evm_hide::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  evm_def* mdl = smart_cast<evm_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;

  result first;
  x.answer = &first;
  for (int i=1; i<np; i++) {
    DCASSERT(pass[i]);
    SafeCompute(pass[i], x);
    DCASSERT(first.isNormal());
    evm_event* e = smart_cast <evm_event*> (first.getPtr());
    DCASSERT(e);
    mdl->HideEvent(pass[i], e);
  }
  x.answer = answer;
}


// ********************************************************
// *                   evm_assert class                   *
// ********************************************************

class evm_assert : public model_internal {
public:
  evm_assert();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

evm_assert::evm_assert() : model_internal(em->VOID, "assert", 2)
{
  SetFormal(1, em->BOOL->addProc(), "b");
  SetRepeat(1);
  SetDocumentation("Define a set of assertions that must be true in each state.  An error message will be displayed if an assertion does not evaluate to true in some state.");
}

void evm_assert::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  evm_def* mdl = smart_cast<evm_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;

  for (int i=1; i<np; i++) {
    if (0==pass[i]) continue;
    mdl->AddAssertion(pass[i]);
  }
  x.answer = answer;
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_evmform : public initializer {
  public:
    init_evmform();
    virtual bool execute();
};
init_evmform the_evmform_initializer;

init_evmform::init_evmform() : initializer("init_evmform")
{
  usesResource("em");
  usesResource("CML");
  buildsResource("formalisms");
}

bool init_evmform::execute()
{
  if (0==em) return false;

  // set up and register intvar types
  simple_type* t_intvar  = new void_type("intvar", "Integer variable", "Integer variable for a generic event-variable model.");
  t_intvar->setPrintable();
  type* t_set_intvar = newSetType("{intvar}", t_intvar);
  em->registerType(t_intvar);
  em->registerType(t_set_intvar);

  // set up and register trans types
  simple_type* t_event  = new void_type("event", "Event", "Event for a generic event-variable model.");
  t_event->setPrintable();
  type* t_set_event = newSetType("{event}", t_event);
  em->registerType(t_event);
  em->registerType(t_set_event);

  // another formalism may have already registered these types.
  // all we care is that they are registered.
  evm_def::intvar_type = em->findType("intvar");
  evm_def::event_type = em->findType("event");
  DCASSERT(evm_def::intvar_type);
  DCASSERT(evm_def::event_type);
  DCASSERT(em->findType("{intvar}"));
  DCASSERT(em->findType("{event}"));


  // Set up and register formalisms
  const char* longdocs = "The event & variable formalism allows manipulation of integer state variables by events.  Event enabling expressions and assignment of state variables by events are specified by hand via the appropriate function calls.";

  formalism* evm = new evm_formalism("evm", "Event & Variable Model", longdocs);
  if (!em->registerType(evm)) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Couldn't register evm type";
      em->stopIO();
    }
    return false;
  }

  // fill symbol table
  symbol_table* evmsyms = MakeSymbolTable();
  evmsyms->AddSymbol(  new evm_eval     );
  evmsyms->AddSymbol(  new evm_range    );
  evmsyms->AddSymbol(  new evm_enabled  );
  evmsyms->AddSymbol(  new evm_assign   );
  evmsyms->AddSymbol(  new evm_init     );
  evmsyms->AddSymbol(  new evm_hide     );
  evmsyms->AddSymbol(  new evm_assert   );
  Add_DSDE_varfuncs(evm_def::intvar_type, evmsyms);
  Add_DSDE_eventfuncs(evm_def::event_type, evmsyms);
  evm->setFunctions(evmsyms); 
  evm->addCommonFuncs(CML);


  // Set up options
  option* debug = em->findOption("Debug");
  evm_def::evm_debug.Initialize(debug,
    "evms",
    "When set, diagnostic messages are displayed regarding evm (event & variable model) construction.",
    false
  );

  option* warning = em->findOption("Warning");
  group_of_named evmwarnings(8);
  evmwarnings.AddItem(evm_def::no_event.Initialize(warning,
    "evm_no_event",
    "For absence of events in event & variable models",
    true
  ));
  evmwarnings.AddItem(evm_def::no_vars.Initialize(warning,
    "evm_no_vars",
    "For absence of variables in event & variable models",
    true
  ));
  evmwarnings.AddItem(evm_def::no_part.Initialize(warning,
    "evm_no_part",
    "If some, but not all, variables are assiged to groups using partition",
    true
  ));
  evmwarnings.AddItem(evm_def::dup_part.Initialize(warning,
    "evm_dup_part",
    "For multiple partition definitions for a variable",
    true
  ));
  evmwarnings.AddItem(evm_def::dup_range.Initialize(warning,
    "evm_dup_range",
    "For duplicate variable ranges in event & variable models",
    true
  ));
  evmwarnings.AddItem(evm_def::dup_assign.Initialize(warning,
    "evm_dup_assign",
    "For multiple assignments on the same variable and event in event & variable models",
    true
  ));
  evmwarnings.AddItem(evm_def::dup_init.Initialize(warning,
    "evm_dup_init",
    "For multiple calls to init for the same variable in event & variable models",
    true
  ));
  evmwarnings.AddItem(evm_def::dup_hide.Initialize(warning,
    "evm_dup_hide",
    "For multiple calls to hide for the same variable in event & variable models",
    true
  ));
  evmwarnings.Finish(warning, "evm_ALL", "Group of all evm warnings");


  radio_button** ms_list = new radio_button*[4];
  ms_list[evm_hlm::INDEXED] = new radio_button(
    "INDEXED",
    "Format is [v1:1, v2:0, v3:2, v4:0, v5:0, v6:1]",
    evm_hlm::INDEXED
  );
  ms_list[evm_hlm::SAFE] = new radio_button(
    "SAFE",
    "Format is [v1, v3:2, v6]",
    evm_hlm::SAFE
  );
  ms_list[evm_hlm::SPARSE] = new radio_button(
    "SPARSE",
    "Format is [v1:1, v3:2, v6:1]",
    evm_hlm::SPARSE
  );
  ms_list[evm_hlm::VECTOR] = new radio_button(
    "VECTOR",
    "Format is [1, 0, 2, 0, 0, 1]",
    evm_hlm::VECTOR
  );
  evm_hlm::StateStyle = evm_hlm::SPARSE;
  em->addOption(
    MakeRadioOption("EVMStateStyle",
      "How to display a state in an event & variable model",
      ms_list, 4, evm_hlm::StateStyle
    )
  );

  return true;
}
