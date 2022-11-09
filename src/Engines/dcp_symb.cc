
// MDD stuff
#include "../Modules/glue_meddly.h"

#include "dcp_symb.h"

#include "../Options/optman.h"
#include "../Options/options.h"

#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_inst.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/measures.h"
#include "../ExprLib/engine.h"

// Formalisms and such
#include "../Formlsms/noevnt_hlm.h"

// Templates
#include "../include/radixsort.h"

#include "../_Timer/timerlib.h"

// #define DEBUG_STUFF
// #define DEBUG_OVERALL
// #define DEBUG_ORDER


// **************************************************************************
// *                                                                        *
// *                           icp_encoder  class                           *
// *                                                                        *
// **************************************************************************

class icp_encoder : public meddly_encoder {
  long* terms;
  int maxbound;
  result ans;
protected:
  no_event_model* nem;
public:
  icp_encoder(const char* n, MEDDLY::forest *f, no_event_model* m);
  icp_encoder(const char* n, MEDDLY::forest *f, no_event_model* m, int mb);
protected:
  virtual ~icp_encoder();
public:
  virtual bool arePrimedVarsSeparate() const { return false; }
  virtual int getNumDDVars() const {
    DCASSERT(nem);
    return nem->NumVars();
  }
  virtual void buildSymbolicSV(const symbol* sv, bool primed,
                                expr* f, shared_object* ans);

  virtual void state2minterm(const shared_state* s, int* mt) const;
  virtual void minterm2state(const int* mt, shared_state *s) const;

  virtual void createMinterms(const int* const* m, int n, shared_object* a);
  virtual void createMinterms(const int* const* f, const int* const* t,
                                int n, shared_object* a);

  virtual meddly_encoder*
  copyWithDifferentForest(const char* n, MEDDLY::forest*) const;
};

// **************************************************************************
// *                          icp_encoder  methods                          *
// **************************************************************************

icp_encoder
::icp_encoder(const char* n, MEDDLY::forest *f, no_event_model* m)
 : meddly_encoder(n, f)
{
  nem = m;

  maxbound = 0;
  int N = nem->NumVars();
  const MEDDLY::domain* d = f->getDomain();
  for (int i=0; i<N; i++) {
    maxbound = MAX(maxbound, d->getVariableBound(i+1));
  }

  // for use with buildSymbolicSV
  terms = new long[maxbound];
}

icp_encoder::icp_encoder(const char* n, MEDDLY::forest *f, no_event_model* m,
  int maxb) : meddly_encoder(n, f)
{
  nem = m;
  maxbound = maxb;
  terms = new long[maxbound];
}

icp_encoder::~icp_encoder()
{
  delete[] terms;
}

void icp_encoder
::buildSymbolicSV(const symbol* sv, bool primed, expr* f, shared_object* answer)
{
  DCASSERT(!primed);
  if (0==sv) throw Failed;
  shared_ddedge* dd = dynamic_cast<shared_ddedge*> (answer);
  if (0==dd) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (dd->numRefs()>1) throw Shared_Output_Edge;
#endif
  DCASSERT(dd);

  DCASSERT(0==f);

  const model_statevar* mv = dynamic_cast<const model_statevar*> (sv);
  DCASSERT(mv);
#ifdef DEVELOPMENT_CODE
  int i = mv->GetIndex();
  DCASSERT(nem->GetVar(i) == mv);
#endif

  for (long i=mv->NumPossibleValues()-1; i>=0; i--) {
    mv->GetValueNumber(i, ans);
    terms[i] = ans.getInt();
  }

  int level = mv->GetIndex()+1;
#ifdef DEBUG_STUFF
  em->cout() << "\t\tlevel is " << level << "\n";
#endif

  try {
    F->createEdgeForVar(level, false, terms, dd->E);
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

void icp_encoder::state2minterm(const shared_state* s, int* mt) const
{
  DCASSERT(0);
  throw Failed;
}

void icp_encoder::minterm2state(const int* mt, shared_state *s) const
{
  DCASSERT(0);
  throw Failed;
}

void icp_encoder::createMinterms(const int* const*, int, shared_object*)
{
  DCASSERT(0);
  throw Failed;
}

void icp_encoder::createMinterms(const int* const*, const int* const*,
                            int, shared_object*)
{
  DCASSERT(0);
  throw Failed;
}

meddly_encoder*
icp_encoder::copyWithDifferentForest(const char* n, MEDDLY::forest* nf) const
{
  return new icp_encoder(n, nf, nem, maxbound);
}

// ******************************************************************
// *                                                                *
// *                     mdd_states_only  class                     *
// *                                                                *
// ******************************************************************

class mdd_states_only : public lldsm {
public:
  MEDDLY::domain* vars;
  MEDDLY::dd_edge legal_states;
  icp_encoder* mdd_wrap;
public:
  mdd_states_only(icp_encoder* wr, MEDDLY::domain* d, const MEDDLY::dd_edge &e);
protected:
  virtual ~mdd_states_only();
  virtual const char* getClassName() const { return "mdd_states_only"; }
};

mdd_states_only
::mdd_states_only(icp_encoder* wr, MEDDLY::domain* d,  const MEDDLY::dd_edge &e)
: lldsm(RSS), legal_states(e)
{
  mdd_wrap = wr;
  vars = d;
}

mdd_states_only::~mdd_states_only()
{
  legal_states.clear();
  MEDDLY::destroyDomain(vars);
  Delete(mdd_wrap);
}


// **************************************************************************
// *                                                                        *
// *                           icp_symbgen  class                           *
// *                                                                        *
// **************************************************************************

/// Symbolic state generator for integer constraint models.
class icp_symbgen : public subengine {

  class constraint_sorter {
      struct expritem {
          expr* constraint;
          bool* deplist;
          int numvars;
        public:
          expritem(int K);
          ~expritem();
          void reset(expr* x, List <symbol> &symlist);
          inline bool isBitSet(int b) const {
            CHECK_RANGE(0, b, numvars);
            DCASSERT(deplist);
            return deplist[b];
          };
          inline expr* getExpr() { return constraint; }
      };
      int numvars;
      expritem** data;
      int dalloc;
      int dused;
    public:
      constraint_sorter(int K);
      ~constraint_sorter();
      void add(expr** x, List <symbol> &symlist);
      inline void reset(expr** x, List <symbol> &symlist) {
        dused = 0;
        add(x, symlist);
      }
      inline bool isBitSet(long i, int b) const {
        CHECK_RANGE(0, i, dused);
        DCASSERT(data);
        DCASSERT(data[i]);
        return data[i]->isBitSet(b);
      };
      inline void swap(long i, long j) {
        CHECK_RANGE(0, i, dused);
        CHECK_RANGE(0, j, dused);
        DCASSERT(data);
        SWAP(data[i], data[j]);
      }
      inline int length() const { return dused; }
      inline expr* get(int i) {
        CHECK_RANGE(0, i, dused);
        DCASSERT(data[i]);
        return data[i]->getExpr();
      }
  };


protected:
  static reporting_msg report;
  static debugging_msg debug;
  static unsigned combine_method;
  static const unsigned ACCUMULATE = 0;
  static const unsigned FOLD       = 1;
  friend class init_dcpsymbolic;
public:
  icp_symbgen();
  virtual~ icp_symbgen();

  static icp_symbgen* getInstance() {
    icp_symbgen* thing = 0;
    if (0==thing) thing = new icp_symbgen;
    return thing;
  }

  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual void RunEngine(hldsm* m, result &);

  // shared_ddedge* ProcessConjunct(expr** cl, int K, traverse_data &x) const;

  shared_ddedge* ProcessConjunct(expr** cl, int K, meddly_encoder* wr) const;

  inline static
  shared_ddedge* Multiply(shared_ddedge** cl, int N, meddly_encoder* wr) {
    switch (combine_method) {
      case ACCUMULATE:
          return wr->accumulate(MEDDLY::MULTIPLY, cl, N, &debug);

      case FOLD:
          return wr->fold(MEDDLY::MULTIPLY, cl, N, &debug);

      default:
          DCASSERT(0);
          return 0;
    };
  }

protected:
  inline bool startGen(const char* name) {
    if (report.startReport()) {
      report.report() << "Generating reachability set for model " << name;
      return true;
    }
    return false;
  };

  inline bool stopGen(const char* n, const timer &w, long mem) {
    if (report.startReport()) {
      report.report() << "Generated ";
      report.report() << " reachability set for model " << n << "\n";
      report.report() << "\t" << w.elapsed_seconds() << " seconds ";
      report.report() << "required for generation\n";
      if (mem >= 0) {
        report.report().Put('\t');
        report.report().PutMemoryCount(mem, 3);
        report.report() << " required for state generation\n";
      }
      return true;
    }
    return false;
  };

};
reporting_msg icp_symbgen::report;
debugging_msg icp_symbgen::debug;
unsigned icp_symbgen::combine_method;

// **************************************************************************
// *                    icp_symbgen helper class methods                    *
// **************************************************************************

icp_symbgen::constraint_sorter::expritem
::expritem(int K)
{
  numvars = K;
  deplist = new bool[K];
  constraint = 0;
}

icp_symbgen::constraint_sorter::expritem
::~expritem()
{
  delete[] deplist;
}

void icp_symbgen::constraint_sorter::expritem
::reset(expr* x, List <symbol> &symlist)
{
  DCASSERT(x);
  constraint = x;
  // Build dependencies
  for (int i=0; i<numvars; i++) deplist[i] = false;
  symlist.Clear();
  x->BuildSymbolList(traverse_data::GetSymbols, 0, &symlist);
  for (int j=symlist.Length()-1; j>=0; j--) {
    model_statevar* s = smart_cast <model_statevar*> (symlist.Item(j));
    DCASSERT(s);
    CHECK_RANGE(0, s->GetIndex(), numvars);
    deplist[s->GetIndex()] = true;
  }
}

icp_symbgen::constraint_sorter
::constraint_sorter(int K)
{
  numvars = K;
  data = 0;
  dalloc = 0;
  dused = 0;
}

icp_symbgen::constraint_sorter
::~constraint_sorter()
{
  for (int i=0; i<dalloc; i++) delete data[i];
  free(data);
}

void icp_symbgen::constraint_sorter
::add(expr** x, List <symbol> &symlist)
{
  // get length of x
  long len = 0;
  long wused = dused;
  long walloc = dalloc;
  while (x[len]) {
    len++;
    wused++;
    if (wused>walloc) {
      walloc = MIN(2*walloc, walloc+1024);
      walloc = MAX(16L, walloc);
    }
  }
  if (walloc > dalloc) {
    expritem** newd = (expritem**) realloc(data, walloc*sizeof(void*));
    DCASSERT(newd);
    data = newd;
    for (long i=dalloc; i<walloc; i++) data[i] = 0;
    dalloc = walloc;
  }
  for (long i=0; i<len; i++) {
    if (0==data[dused]) data[dused] = new expritem(numvars);
    data[dused]->reset(x[i], symlist);
    dused++;
  }
}

// **************************************************************************
// *                          icp_symbgen  methods                          *
// **************************************************************************

icp_symbgen::icp_symbgen() : subengine()
{
}

icp_symbgen::~icp_symbgen()
{
}

bool icp_symbgen::AppliesToModelType(hldsm::model_type mt) const
{
  return (hldsm::No_Events == mt);
}

void icp_symbgen::RunEngine(hldsm* hm, result &)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));
  if (hm->GetProcess())  return;  // already has SS?

  no_event_model* nem = smart_cast <no_event_model*> (hm);
  DCASSERT(nem);
  DCASSERT(nem->NumVars()>0);

  timer watch;
  if (startGen(hm->Name())) {
    em->report().Put('\n');
    em->stopIO();
  }

  int N = nem->NumVars();
  MEDDLY::variable** vars = new MEDDLY::variable* [N+1];
  vars[0] = 0;
  for (int i=0; i<N; i++) {
    model_statevar* mv = nem->GetVar(i);
    DCASSERT(mv->HasBounds());
    char* name = strdup(mv->Name());
    vars[i+1] = MEDDLY::createVariable(mv->NumPossibleValues(), name);
  }

#ifdef DEBUG_STUFF
  em->cout() << "Inside symbolic RunEngine\n";
  em->cout() << "Building domain\n";
#endif

  MEDDLY::domain* d;
  try {
    d = MEDDLY::createDomain(vars, N);
  }
  catch (MEDDLY::error de) {
    if (hm->StartError(0)) {
      em->cerr() << "Error adding variables: ";
      em->cerr() << de.getName();
      hm->DoneError();
    }
    MEDDLY::destroyDomain(d);
    throw Engine_Failed;
  }

#ifdef DEBUG_STUFF
  em->cout() << "MDD variable order:\n\tterminals\n";
  for (int i=0; i<N; i++) {
    model_statevar* mv = nem->GetVar(i);
    em->cout() << "\t" << mv->Name();
    em->cout()<< " (index " << mv->GetIndex() << ")\n";
  }
  em->cout().flush();
  d->showInfo(em->Fstdout());
  em->cout() << "Building forest\n";
#endif
  MEDDLY::forest::policies p(false);
  p.setPessimistic();
  MEDDLY::forest* f = d->createForest(
    false, MEDDLY::forest::INTEGER, MEDDLY::forest::MULTI_TERMINAL, p
  );
  DCASSERT(f);
  icp_encoder* ddlwrap = new icp_encoder("MDD", f, nem);

  shared_ddedge** cbylevel = new shared_ddedge*[N];
  for (int k=0; k<N; k++) {
    if (debug.startReport()) {
      debug.report() << "Generating level " << k+1 << " constraints:\n";
      debug.stopIO();
    }
    expr** clist = nem->GetConstraintsAtLevel(k);
    cbylevel[k] = ProcessConjunct(clist, k+1, ddlwrap);
  } // for k

  if (debug.startReport()) {
    debug.report() << "Combining level-wise constraints\n";
    debug.stopIO();
  }

  MEDDLY::dd_edge constraints(f);
  shared_ddedge* ans = Multiply(cbylevel, N, ddlwrap);
  if (0==ans) {
    f->createEdge(long(1), constraints);
  } else {
    constraints = ans->E;
    Delete(ans);
  }

#ifdef DEBUG_OVERALL
  em->cout() << "Overall contraints: " << constraints.getNode() << "\n";
  em->cout().flush();
  constraints.show(em->Fstdout(), 2);
  f->showInfo(em->Fstdout());
#endif

  if (stopGen(hm->Name(), watch, f->getPeakMemoryUsed())) {
    em->report() << "\t" << constraints.getCardinality();
    em->report() << " states generated\n";
    em->report() << "\tCurrent nodes: " << f->getCurrentNumNodes() << "\n";
    em->report() << "\tPeak    nodes: " << f->getPeakNumNodes() << "\n";
    em->report() << "\tCurrent memory: ";
    em->report().PutMemoryCount(f->getCurrentMemoryUsed(), 3);
    em->report() << "\n";
    em->report() << "\tPeak    memory: ";
    em->report().PutMemoryCount(f->getPeakMemoryUsed(), 3);
    em->report() << "\n";
    em->stopIO();
    // constraints.show(em->Fstdout(), 1);
  }

  hm->SetProcess(new mdd_states_only(ddlwrap, d, constraints));
}

shared_ddedge* icp_symbgen
::ProcessConjunct(expr** clist, int K, meddly_encoder* wr) const
{
  if (0==clist) return 0;
  result foo;
  traverse_data x(traverse_data::BuildDD);
  x.answer = &foo;
  x.ddlib = wr;
  List <symbol> symlist;
  constraint_sorter COLL(K);
  COLL.add(clist, symlist);
#ifdef DEBUG_ORDER
  em->cout() << "Ordering constraints\n";
  em->cout().flush();
#endif
  radix_sort(COLL, 0, COLL.length(), K-1, false);

  shared_ddedge** edgelist = new shared_ddedge*[COLL.length()];
  for (long i=0; i<COLL.length(); i++) {
#ifdef DEBUG_ORDER
    em->cout() << "\t(";
    em->cout().Put(i, 4);
    em->cout() << ") Traversing ";
    COLL.get(i)->Print(em->cout(), 0);
    em->cout() << "\n";
    em->cout().flush();
#endif
    COLL.get(i)->Traverse(x);
    if (x.answer->isNull()) {
      em->cout() << "\t\tGot null result\n";
      continue;
    }

    edgelist[i] = Share(dynamic_cast <shared_ddedge*> (x.answer->getPtr()));
    if (0==edgelist[i]) {
      em->cout() << "\t\tUnexpected result, not dd edge?\n";
      continue;
    }
  } // for i

  return Multiply(edgelist, COLL.length(), wr);
}

// **************************************************************************
// *                                                                        *
// *                         icp_mdd_analyzer class                         *
// *                                                                        *
// **************************************************************************

// abstract base class for min, max, sat engines
class icp_mdd_analyzer : public subengine {
  static engtype* SSGen;
  friend class init_dcpsymbolic;
public:
  icp_mdd_analyzer();
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual void SolveMeasure(hldsm* m, measure* what);
protected:
  virtual void SolveImplicit(no_event_model* nem,
                              mdd_states_only* mdd, measure* what) = 0;
};

engtype* icp_mdd_analyzer::SSGen = 0;

icp_mdd_analyzer::icp_mdd_analyzer() : subengine()
{
}

bool icp_mdd_analyzer::AppliesToModelType(hldsm::model_type mt) const
{
  return (hldsm::No_Events == mt);
}

void icp_mdd_analyzer::SolveMeasure(hldsm* hm, measure* what)
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

  // Are we stored using MDDs?
  mdd_states_only* msom;
  msom = dynamic_cast <mdd_states_only*> (hm->GetProcess());
  if (msom) {
    return SolveImplicit(nem, msom, what);
  }

  // constraints are in the wrong format.
  // TBD: Print an error message.

  throw No_Engine;
}

// **************************************************************************
// *                                                                        *
// *                           icp_mdd_min  class                           *
// *                                                                        *
// **************************************************************************

class icp_mdd_min : public icp_mdd_analyzer {
public:
  icp_mdd_min();

  static icp_mdd_min* getInstance() {
    static icp_mdd_min* thing = 0;
    if (0==thing) thing = new icp_mdd_min;
    return thing;
  }
protected:
  virtual void SolveImplicit(no_event_model* nem,
                              mdd_states_only* mdd, measure* what);
};

icp_mdd_min::icp_mdd_min() : icp_mdd_analyzer()
{
}

void icp_mdd_min
::SolveImplicit(no_event_model* nem, mdd_states_only* mdd, measure* what)
{
  DCASSERT(mdd);
  DCASSERT(what);
  result foo;
  foo.setNull();
  traverse_data x(traverse_data::BuildDD);
  x.answer = &foo;
  x.ddlib = mdd->mdd_wrap;

  what->TraverseRHS(x);
  if (foo.isNull()) {
    what->SetNull();
    return;
  }

  shared_ddedge* me = Share(dynamic_cast <shared_ddedge*> (foo.getPtr()));
  if (0==me) {
    em->cout() << "\t\tUnexpected result, not dd edge?\n";
    throw Engine_Failed;
  }

  // Find minimum value, over constraints

  if (mdd->legal_states.getNode()==0) {
    // constraints cannot be satisfied
    what->SetNull();
    return;
  }

  long a, z;
  MEDDLY::apply(MEDDLY::MIN_RANGE, me->E, a);
  MEDDLY::apply(MEDDLY::MAX_RANGE, me->E, z);

  // em->cout() << "Unconstrained range is " << a << " .. " << z << "\n";

  MEDDLY::forest* f = me->E.getForest();

  // g := if (constraints) then a else z;
  //
  MEDDLY::dd_edge g(f);
  MEDDLY::dd_edge c_amz(f);
  MEDDLY::dd_edge c_z(f);
  f->createEdge(long(a-z), c_amz);
  f->createEdge(long(z), c_z);
  g = mdd->legal_states * c_amz;
  g += c_z;

/*
  em->cout() << "constraints:\n";
  em->cout().flush();
  mdd->legal_states.show(stdout, 2);
  em->cout() << "0->a 1->z:\n";
  em->cout().flush();
  g.show(stdout, 2);
*/

  // g := if (constraints) then me else a;
  MEDDLY::apply(MEDDLY::MAXIMUM, g, me->E, g);

  // find min of me->E with constraints satisfied
  long m;
  MEDDLY::apply(MEDDLY::MIN_RANGE, g, m);
  foo.setReal(m);
  what->SetValue(foo);

  // All the rest is to show one of the minima

  // get set of minterms that satisfy constraints and produce the minimum, as
  // w := (g==m) * constraints
  MEDDLY::dd_edge c_m(f);
  MEDDLY::dd_edge w(f);
  f->createEdge(long(m), c_m);
  MEDDLY::apply(MEDDLY::EQUAL, g, c_m, w);

  // display one of the states
  MEDDLY::enumerator iter(w);
  DCASSERT(iter);
  const int* minterm = iter.getAssignments();

  // Convert minterm to state
  for (int i=nem->NumVars()-1; i>=0; i--) {
    model_statevar* sv = nem->GetVar(i);
    int level = sv->GetIndex()+1;
    CHECK_RANGE(1, level, nem->NumVars()+1);
    sv->SetToValueNumber(minterm[level]);
  } // for i

  em->cout() << "Minimum value " << m << " obtainted in state ";
  nem->ShowCurrentState(em->cout());
  em->cout() << "\n";

  Delete(me);
}


// **************************************************************************
// *                                                                        *
// *                           icp_mdd_max  class                           *
// *                                                                        *
// **************************************************************************

class icp_mdd_max : public icp_mdd_analyzer {
public:
  icp_mdd_max();
  static icp_mdd_max* getInstance() {
    static icp_mdd_max* thing = 0;
    if (0==thing) thing = new icp_mdd_max;
    return thing;
  }
protected:
  virtual void SolveImplicit(no_event_model* nem,
                              mdd_states_only* mdd, measure* what);
};

icp_mdd_max::icp_mdd_max() : icp_mdd_analyzer()
{
}

void icp_mdd_max
::SolveImplicit(no_event_model* nem, mdd_states_only* mdd, measure* what)
{
  DCASSERT(mdd);
  DCASSERT(what);
  result foo;
  foo.setNull();
  traverse_data x(traverse_data::BuildDD);
  x.answer = &foo;
  x.ddlib = mdd->mdd_wrap;

  what->TraverseRHS(x);
  if (foo.isNull()) {
    what->SetNull();
    return;
  }

  shared_ddedge* me = dynamic_cast <shared_ddedge*> (foo.getPtr());
  if (0==me) {
    em->cout() << "\t\tUnexpected result, not dd edge?\n";
    throw Engine_Failed;
  }

  // Find maximum value, over constraints

  if (mdd->legal_states.getNode()==0) {
    // constraints cannot be satisfied
    what->SetNull();
    return;
  }

  long a, z;
  MEDDLY::apply(MEDDLY::MIN_RANGE, me->E, a);
  MEDDLY::apply(MEDDLY::MAX_RANGE, me->E, z);

  MEDDLY::forest* f = me->E.getForest();

  // g := if (constraints) then z else a;
  //
  MEDDLY::dd_edge g(f);
  MEDDLY::dd_edge c_zma(f);
  MEDDLY::dd_edge c_a(f);
  f->createEdge(long(z-a), c_zma);
  f->createEdge(long(a), c_a);
  g = mdd->legal_states * c_zma;
  g += c_a;

  // g := if (constraints) then me else a;
  MEDDLY::apply(MEDDLY::MINIMUM, g, me->E, g);

  // find min of me->E with constraints satisfied
  long m;
  MEDDLY::apply(MEDDLY::MAX_RANGE, g, m);
  foo.setReal(m);
  what->SetValue(foo);

  // All the rest is to show one of the maxima

  // get set of minterms that satisfy constraints and produce the maximum, as
  // w := (g==m) * constraints
  MEDDLY::dd_edge c_m(f);
  MEDDLY::dd_edge w(f);
  f->createEdge(long(m), c_m);
  MEDDLY::apply(MEDDLY::EQUAL, g, c_m, w);

  // display one of the states
  MEDDLY::enumerator iter(w);
  DCASSERT(iter);
  const int* minterm = iter.getAssignments();

  // Convert minterm to state
  for (int i=nem->NumVars()-1; i>=0; i--) {
    model_statevar* sv = nem->GetVar(i);
    int level = sv->GetIndex()+1;
    CHECK_RANGE(1, level, nem->NumVars()+1);
    sv->SetToValueNumber(minterm[level]);
  } // for i

  em->cout() << "Minimum value " << m << " obtainted in state ";
  nem->ShowCurrentState(em->cout());
  em->cout() << "\n";

  Delete(me);
}


// **************************************************************************
// *                                                                        *
// *                           icp_mdd_sat  class                           *
// *                                                                        *
// **************************************************************************

class icp_mdd_sat : public icp_mdd_analyzer {
public:
  icp_mdd_sat();
  static icp_mdd_sat* getInstance() {
    static icp_mdd_sat* thing = 0;
    if (0==thing) thing = new icp_mdd_sat;
    return thing;
  }
protected:
  virtual void SolveImplicit(no_event_model* nem,
                              mdd_states_only* mdd, measure* what);
};

icp_mdd_sat::icp_mdd_sat() : icp_mdd_analyzer()
{
}

void icp_mdd_sat
::SolveImplicit(no_event_model* nem, mdd_states_only* mdd, measure* what)
{
  DCASSERT(mdd);
  DCASSERT(what);
  result foo;
  foo.setNull();
  traverse_data x(traverse_data::BuildDD);
  x.answer = &foo;
  x.ddlib = mdd->mdd_wrap;

  what->TraverseRHS(x);
  if (foo.isNull()) {
    what->SetNull();
    return;
  }

  shared_ddedge* me = Share(dynamic_cast <shared_ddedge*> (foo.getPtr()));
  if (0==me) {
    em->cout() << "\t\tUnexpected result, not dd edge?\n";
    throw Engine_Failed;
  }

  // AND this with the constraints
  me->E *= mdd->legal_states;

  foo.setBool(me->E.getNode()>0);
  what->SetValue(foo);
  if (!foo.getBool()) {
    Delete(me);
    return;
  }

  // display one of the states
  MEDDLY::enumerator iter(me->E);
  DCASSERT(iter);
  const int* minterm = iter.getAssignments();

  // Convert minterm to state
  for (int i=nem->NumVars()-1; i>=0; i--) {
    model_statevar* sv = nem->GetVar(i);
    int level = sv->GetIndex()+1;
    CHECK_RANGE(1, level, nem->NumVars()+1);
    sv->SetToValueNumber(minterm[level]);
  } // for i

  em->cout() << "Satisfiable in state ";
  nem->ShowCurrentState(em->cout());
  em->cout() << "\n";

  Delete(me);
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_dcpsymbolic : public initializer {
  public:
    init_dcpsymbolic();
    virtual bool execute();
};
init_dcpsymbolic the_dcpsymbolic_initializer;

init_dcpsymbolic::init_dcpsymbolic() : initializer("init_dcpsymbolic")
{
  usesResource("em");
  usesResource("engtypes");
}

bool init_dcpsymbolic::execute()
{
  if (0==em) return false;

  // Initialize libraries
  // InitMEDDLy(em);

  // Initialize options
  icp_symbgen::report.initialize(em->OptMan(), "implicit_dcp_gen",
    "When set, implicit reachability set performance is reported."
  );
  icp_symbgen::debug.initialize(em->OptMan(), "implicit_dcp_gen",
    "When set, implicit reachability set details are displayed."
  );

  // accumulate vs. fold option
  icp_symbgen::combine_method = icp_symbgen::FOLD;

  if (em->OptMan()) {
    option* mccm = em->OptMan()->addRadioOption(
      "MeddlyConstraintCombinationMethod",
      "How to combine constraints when MEDDLY is used for ImplicitDCSolve",
      2, icp_symbgen::combine_method
    );
    mccm->addRadioButton(
      "ACCUMULATE",
      "Combine MDDs in order",
      icp_symbgen::ACCUMULATE
    );
    mccm->addRadioButton(
      "FOLD",
      "Pairwise combine small MDDs together; repeat",
      icp_symbgen::FOLD
    );
  }

  // Register engines
  icp_mdd_analyzer::SSGen = em->findEngineType("ImplicitDCSolve");
  DCASSERT(icp_mdd_analyzer::SSGen);
  RegisterEngine(
    icp_mdd_analyzer::SSGen,
    "MEDDLY",
    "Logical manipulation using Meddly.",
    icp_symbgen::getInstance()
  );

  RegisterEngine(em,
      "MinExpr",
      "IMPLICIT",
      "Builds constraints and expression implicitly (using MDDs), find minimim value in MDD",
      icp_mdd_min::getInstance()
  );
  RegisterEngine(em,
      "MaxExpr",
      "IMPLICIT",
      "Builds constraints and expression implicitly (using MDDs), find maximum value in MDD",
      icp_mdd_max::getInstance()
  );
  RegisterEngine(em,
      "SatExpr",
      "IMPLICIT",
      "Builds constraints and expression implicitly (using MDDs), check for satisfiability",
      icp_mdd_sat::getInstance()
  );

  return true;
}


