
// $Id$

#include "gen_meddly.h"

#include "../Options/options.h"

#include "../ExprLib/startup.h"
#include "../Modules/glue_meddly.h"
#include "../Modules/expl_states.h"
#include "../Formlsms/dsde_hlm.h"
#include "../Formlsms/rss_meddly.h"

// #define DUMP_MXD
// #define DUMP_MTMXD
// #define DEBUG_BUILD_SV
// #define DEBUG_SHAREDEDGE
// #define DEBUG_ENCODE
// #define DEBUG_EVENT_CONSTR
// #define DEBUG_EVENT_OVERALL
// #define DEBUG_NOCHANGE

// #define SHOW_SUBSTATES
// #define SHOW_MINTERMS

// #define SHORT_CIRCUIT_ENABLING

#define USING_MEDDLY_ADD_MINTERM

using namespace MEDDLY;

// **************************************************************************
// *                                                                        *
// *                          minterm_pool methods                          *
// *                                                                        *
// **************************************************************************

minterm_pool::minterm_pool(int max_minterms, int depth)
{
  term_depth = depth;
  alloc = max_minterms+1;
  free_list = 0;
  minterms = new int*[alloc];
  for (int i=0; i<alloc; i++) minterms[i] = 0;
  used = 1;
}

minterm_pool::~minterm_pool()
{
  for (int i=1; i<used; i++) delete[] minterms[i];
  delete[] minterms;
}

int* minterm_pool::allocMinterm()
{
  CHECK_RANGE(0, used, alloc);
  int* answer = new int[term_depth+2];
  answer[term_depth] = 1;
  answer[term_depth+1] = used;
  minterms[used] = answer;
  used++;
  return answer;
}

void minterm_pool::reportStats(DisplayStream &out) const
{
  out << "\t" << used << " minterms used, required ";
  size_t batchmem = (term_depth+2)*sizeof(int)*used + alloc*sizeof(int*);
  out.PutMemoryCount(batchmem, 3);
  out << "\n";
}

// **************************************************************************
// *                                                                        *
// *                        meddly_varoption methods                        *
// *                                                                        *
// **************************************************************************

#ifdef USING_OLD_MEDDLY_STUFF

bool meddly_varoption::vars_named;

meddly_varoption::meddly_varoption(meddly_reachset &x, const dsde_hlm &p)
: parent(p), ms(x)
{
  mxd_wrap = 0;
  built_ok = true;
  event_enabling = new dd_edge*[parent.getNumEvents()];
  event_firing = new dd_edge*[parent.getNumEvents()];
  for (int i=0; i<parent.getNumEvents(); i++) {
    event_enabling[i] = event_firing[i] = 0;
  }
}

meddly_varoption::~meddly_varoption()
{
  if (event_enabling) {
    for (int i=0; i<parent.getNumEvents(); i++) delete event_enabling[i];
    delete[] event_enabling;
  }
  if (event_firing) {
    for (int i=0; i<parent.getNumEvents(); i++) delete event_firing[i];
    delete[] event_firing;
  }
  Delete(mxd_wrap);
}

void meddly_varoption::initializeVars()
{
  // We're good, just need to set the initial states

  // allocate what we need
  shared_state* st = new shared_state(&parent);
  int num_init = parent.NumInitialStates();
  int** mt = new int* [num_init];
#ifdef DEBUG_SHAREDEDGE
  fprintf(stderr, "initial: ");
#endif

  // Convert initial states to minterms
  for (int n=0; n<num_init; n++) {
    mt[n] = new int[ms.getNumLevels()];
    parent.GetInitialState(n, st);
    ms.MddState2Minterm(st, mt[n]);
  } // for n

  // Build DD from minterms
  shared_ddedge* initial = ms.newMddEdge();
  DCASSERT(initial);
  try {
    ms.createMinterms(mt, num_init, initial);
    ms.setInitial(initial);

    // Cleanup
    for (int n=0; n<num_init; n++)  delete[] mt[n];
    delete[] mt;
    Delete(st);
    return;
  }
  catch (sv_encoder::error e) {
    // Cleanup
    for (int n=0; n<num_init; n++)  delete[] mt[n];
    delete[] mt;
    Delete(st);
    switch (e) {
      case sv_encoder::Out_Of_Memory:   throw subengine::Out_Of_Memory;
      default:                          throw subengine::Engine_Failed;
    }
  }
}

char* meddly_varoption::buildVarName(const hldsm::partinfo &part, int k)
{
  if (!vars_named) return 0;
  StringStream s;
  bool printed = false;
  for (int p=part.pointer[k]; p>part.pointer[k-1]; p--) {
     if (printed)  s << ":";
     else          printed = true;
     s << part.variable[p]->Name();
  }  // for p
  return s.GetString();
}

void meddly_varoption::reportStats(DisplayStream &out) const
{
  ms.reportStats(out);
  if (mxd_wrap) mxd_wrap->reportStats(out);
}

satotf_opname::otf_relation* meddly_varoption::buildNSF_OTF(named_msg &debug)
{
  return 0;
}

// **************************************************************************
// *                                                                        *
// *                         bounded_encoder  class                         *
// *                                                                        *
// **************************************************************************

class bounded_encoder : public meddly_encoder {
  int* terms;
  int maxbound;
  long lastcomputed;
  traverse_data tdx;
  result ans;
  const hldsm &parent;
  shared_state* expl_state;
public:
  bounded_encoder(const char* n, forest* f, const hldsm &p, int max_var_size);
protected:
  virtual ~bounded_encoder();
public:
  virtual bool arePrimedVarsSeparate() const { return false; }
  virtual int getNumDDVars() const { return parent.getPartInfo().num_levels; }
  virtual void buildSymbolicSV(const symbol* sv, bool primed, 
                                expr *f, shared_object* answer);

  virtual void state2minterm(const shared_state* s, int* mt) const;
  virtual void minterm2state(const int* mt, shared_state *s) const;

  virtual meddly_encoder* copyWithDifferentForest(const char*, forest*) const;
protected:
  void FillTerms(const model_statevar* sv, int p, int &i, expr* f);
};

// **************************************************************************
// *                                                                        *
// *                        bounded_encoder  methods                        *
// *                                                                        *
// **************************************************************************

bounded_encoder
::bounded_encoder(const char* n, forest* f, const hldsm &p, int max_var_size)
: meddly_encoder(n, f), tdx(traverse_data::Compute), parent(p)
{
  DCASSERT(parent.hasPartInfo());
  maxbound = MAX(parent.getPartInfo().num_levels+1, max_var_size);
  terms = new int[maxbound];
  expl_state = new shared_state(&p);
  tdx.answer = &ans;
  tdx.current_state = expl_state;
}

bounded_encoder::~bounded_encoder()
{
  delete[] terms;
  Delete(expl_state);
}

void bounded_encoder
::buildSymbolicSV(const symbol* sv, bool primed, expr* f, shared_object* answer)
{
  if (0==sv) throw Failed;
  shared_ddedge* dd = dynamic_cast<shared_ddedge*> (answer);
  if (0==dd) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (dd->numRefs()>1) throw Shared_Output_Edge;
#endif
  DCASSERT(dd);

  const model_statevar* mv = dynamic_cast<const model_statevar*> (sv);
  DCASSERT(mv);

  int level = mv->GetPart();
  CHECK_RANGE(1, level, 1+parent.getPartInfo().num_levels);

  int i = 0;
  FillTerms(mv, parent.getPartInfo().pointer[level], i, f);

  try {
    F->createEdgeForVar(level, primed, terms, dd->E);

#ifdef DEBUG_BUILD_SV
    fprintf(stderr, "%#010lx: ", (unsigned long)answer);
    fprintf(stderr, " built variable %s", mv->Name());
    if (primed) fprintf(stderr, "'");
    fprintf(stderr, " (level %d", level);
    if (primed) fprintf(stderr, "'");
    fprintf(stderr, "): ");
    dd->E.show(stderr, 0);
    fprintf(stderr, "\n");
#endif
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

void bounded_encoder
::FillTerms(const model_statevar* sv, int p, int &i, expr* f)
{
  DCASSERT(sv);
  if (parent.getPartInfo().pointer[sv->GetPart()-1] >= p) {
    CHECK_RANGE(0, i, maxbound);
    terms[i] = lastcomputed;
    i++;
    return;
  }
  long stop = sv->NumPossibleValues();
  if (parent.getPartInfo().variable[p] != sv) {
    for (long v=0; v<stop; v++) {
      FillTerms(sv, p-1, i, f);
    } // for v
    return;
  }
  for (long v=0; v<stop; v++) {
    if (f) {
      sv->GetValueNumber(v, ans);
      sv->SetNextState(tdx, expl_state, ans.getInt());
      f->Compute(tdx);
    } else {
      sv->GetValueNumber(v, ans);
    }
    lastcomputed = ans.getInt();
    FillTerms(sv, p-1, i, f);
  } // for v
}

void 
bounded_encoder::state2minterm(const shared_state* s, int* mt) const
{
  if (0==s || 0==mt) throw Failed;

  const hldsm::partinfo &part = parent.getPartInfo();
  int p = part.pointer[part.num_levels];
  for (int k=part.num_levels; k; k--) {
    int stop = part.pointer[k-1];
    mt[k] = 0;
    for (; p> stop; p--) {
      mt[k] *= part.variable[p]->NumPossibleValues();
      mt[k] += s->get(part.variable[p]->GetIndex());
    } // for p
  } // for k
}

void
bounded_encoder::minterm2state(const int* mt, shared_state *s) const
{
  if (0==s || 0==mt) throw Failed;

  int p = 0;
  const hldsm::partinfo &part = parent.getPartInfo();
  for (int k=1; k<= part.num_levels; k++) {
    int stop = part.pointer[k];
    int digit = mt[k];
    for (; p <= stop; p++) {
      int b = part.variable[p]->NumPossibleValues();
      s->set(part.variable[p]->GetIndex(), digit % b);
      digit /= b;
    } // for p
  } // for k
}

meddly_encoder* 
bounded_encoder::copyWithDifferentForest(const char* n, forest* nf) const
{
  return new bounded_encoder(n, nf, parent, maxbound);
}


// **************************************************************************
// *                                                                        *
// *                        bounded_varoption  class                        *
// *                                                                        *
// **************************************************************************

class bounded_varoption : public meddly_varoption {
  int* minterm; // unprimed minterm
  int* minprim; // primed minterm
protected:
  meddly_encoder* mtmxd_wrap;
public:
  bounded_varoption(meddly_reachset &x, const dsde_hlm &p, const exprman* em,
    const meddly_procgen &pg);

  virtual ~bounded_varoption();

  virtual void initializeEvents(named_msg &d);

  virtual void updateEvents(named_msg &d, bool* cl);

  virtual bool hasChangedLevels(const dd_edge &s, bool* cl);

  virtual void reportStats(DisplayStream &out) const;

protected: // in the following, dd is an mxd edge.
  void encodeExpr(expr* e, dd_edge &dd, const char *what, const char* who);

  void buildNoChange(const model_event &e, dd_edge &dd);

private:
  void checkBounds(const exprman* em);
  int  initDomain(const exprman* em);
  void initEncoders(int maxbound, const meddly_procgen &pg);

};

// **************************************************************************
// *                                                                        *
// *                       bounded_varoption  methods                       *
// *                                                                        *
// **************************************************************************

bounded_varoption
::bounded_varoption(meddly_reachset &x, const dsde_hlm &p, 
  const exprman* em, const meddly_procgen &pg) : meddly_varoption(x, p)
{
  minterm = 0;
  minprim = 0;
  mtmxd_wrap = 0;

  checkBounds(em);
  int maxbounds = initDomain(em);
  initEncoders(maxbounds, pg);
}

bounded_varoption::~bounded_varoption()
{
  delete[] minterm;
  delete[] minprim;
  Delete(mtmxd_wrap);
}

void bounded_varoption::initializeEvents(named_msg &d)
{
  // Nothing to do?
}

void bounded_varoption::updateEvents(named_msg &d, bool* cl)
{
  DCASSERT(built_ok);
  DCASSERT(event_enabling);
  DCASSERT(event_firing);
  forest* f = get_mxd_forest();
  DCASSERT(f);
  dd_edge mask(f);

  // For now, don't bother checking cl; just build everything

  for (int i=0; i<getParent().getNumEvents(); i++) {
    const model_event* e = getParent().readEvent(i);
    DCASSERT(e);

    //
    // Build enabling for this event
    //

    if (d.startReport()) {
      d.report() << "Building enabling   DD for event ";
      d.report() << e->Name() << "\n";
      d.stopIO();
    }

    if (0==event_enabling[i]) event_enabling[i] = new dd_edge(f);
    encodeExpr(
      e->getEnabling(), *event_enabling[i], "enabling of event", e->Name()
    );

    //
    // Build firing for this event
    //

    if (d.startReport()) {
      d.report() << "Building next-state DD for event ";
      d.report() << e->Name() << "\n";
      d.stopIO();
    }

    if (0==event_firing[i])   event_firing[i]   = new dd_edge(f);
    encodeExpr(
      e->getNextstate(), *event_firing[i], "firing of event", e->Name()
    );

    // Build "don't change" mask for this event; "AND" it with firing
    buildNoChange(*e, mask);

#ifdef DEBUG_EVENT_CONSTR
    printf("Don't change mask for event %s (#%d):\n", e->Name(), i);
    mask.show(stdout, 2);
    printf("Raw next state for event %s:\n", e->Name());
    event_firing[i]->show(stdout, 2);
#endif

    (*event_firing[i]) *= mask;

#ifdef DEBUG_EVENT_OVERALL
    printf("Final next state for event %s:\n", e->Name());
    event_firing[i]->show(stdout, 2);
#endif
  } // for i
}

bool bounded_varoption::hasChangedLevels(const dd_edge &s, bool* cl)
{
  return false;
}




void bounded_varoption::reportStats(DisplayStream &out) const
{
  meddly_varoption::reportStats(out);
  if (mtmxd_wrap) mtmxd_wrap->reportStats(out);
#ifdef DUMP_MXD
  out << "NSF root in MXD: " << ms.nsf->E.getNode() << "\n";
  out << "MXD forest:\n";
  out.flush();
  ms.mxd_wrap->getForest()->showInfo(out.getDisplay(), 1);
  fflush(out.getDisplay());
#endif
#ifdef DUMP_MTMXD
  out << "MTMXD forest:\n";
  out.flush();
  mtmxd_wrap->getForest()->showInfo(out.getDisplay(), 1);
  fflush(out.getDisplay());
#endif

}

void bounded_varoption
::encodeExpr(expr* e, dd_edge &dd, const char* what, const char* who)
{
  DCASSERT(mtmxd_wrap);
  if (0==e) {
    forest* f = dd.getForest();
    f->createEdge(true, dd);
    return;
  }
  DCASSERT(e);
  result foo;
  traverse_data x(traverse_data::BuildDD);
  x.answer = &foo;
  x.ddlib = mtmxd_wrap;

  // First, build the expr as an MTMXD
  e->Traverse(x);
  if (foo.isNull()) {
    if (getParent().StartError(0)) {
      getParent().SendError("Got null result for ");
      getParent().SendError(what);
      getParent().SendError(who);
      getParent().DoneError();
    }
    throw subengine::Engine_Failed;
  }
  shared_ddedge* me = smart_cast <shared_ddedge*>(foo.getPtr());
  DCASSERT(me);

#ifdef DEBUG_ENCODE
  getParent().getEM()->cout() << "MTMXD encoding for expr: ";
  e->Print(getParent().getEM()->cout(), 0);
  getParent().getEM()->cout() << " is node " << me->E.getNode() << "\n";
  getParent().getEM()->cout().flush();
  me->E.show(stdout, 2);
#endif

  // now, copy it into the MXD
  try {
    apply(COPY, me->E, dd);
    return;
  }
  catch (error ce) {
    // An error occurred, report it...
    if (getParent().StartError(0)) {
      getParent().SendError("Meddly error ");
      getParent().SendError(ce.getName());
      getParent().SendError(" for ");
      getParent().SendError(what);
      getParent().SendError(who);
      getParent().DoneError();
    }

    // ...and figure out which one
    if (error::INSUFFICIENT_MEMORY == ce.getCode())
      throw subengine::Out_Of_Memory;

    throw subengine::Engine_Failed;
  }
}

void bounded_varoption::buildNoChange(const model_event &e, dd_edge &dd)
{
  DCASSERT(minterm);
  DCASSERT(mtmxd_wrap);
  const hldsm::partinfo &part = getParent().getPartInfo();

  // "don't change" levels
  for (int k=1; k<=part.num_levels; k++) {
    minterm[k] = DONT_CARE;
    minprim[k] = (e.nextstateDependsOnLevel(k)) 
        ? DONT_CARE 
        : DONT_CHANGE;
  }
  DCASSERT(dd.getForest());
  try {
    dd.getForest()->createEdge(&minterm, &minprim, 1, dd);
  }
  catch (error fe) {
    if (error::INSUFFICIENT_MEMORY == fe.getCode()) 
      throw subengine::Out_Of_Memory;
    throw subengine::Engine_Failed;
  }

  // Now, "and" in the "don't change" variables
  // that are associated with "do change" levels, if any

  shared_ddedge* x = smart_cast <shared_ddedge*>(mtmxd_wrap->makeEdge(0));
  DCASSERT(x);
  shared_ddedge* xp = smart_cast <shared_ddedge*>(mtmxd_wrap->makeEdge(0));
  DCASSERT(xp);
  dd_edge mask(x->E.getForest());
  mask.getForest()->createEdge(1, mask);
  dd_edge noch(x->E.getForest());

  try {
    // IMPORTANT: iterate from bottom level up, this helps
    // to minimize the number of temporary nodes in the MDD
    for (int k=1; k<=part.num_levels; k++) if (e.nextstateDependsOnLevel(k)) {
      // check variables at this level
      for (int p=part.pointer[k-1]+1; p<=part.pointer[k]; p++) {
        const model_statevar* sv = part.variable[p];
        if (e.nextstateDependsOnVar(sv->GetIndex())) continue;
        // still here?
        // event e depends on level k, but not on state variable sv.
        mtmxd_wrap->buildSymbolicSV(sv, false, 0, x);
        mtmxd_wrap->buildSymbolicSV(sv, true, 0, xp);

        apply(EQUAL, x->E, xp->E, noch);

        mask *= noch;
      } // for p
    } // for k that e depends on

    // cleanup
    Delete(x);
    Delete(xp);
    x=0;
    xp=0;

    // copy mask into the right forest
    dd_edge mask2(dd.getForest());
    apply(COPY, mask, mask2);

#ifdef DEBUG_NOCHANGE
    printf("mask2:\n");
    mask2.show(stdout, 2);
    printf("dd initially:\n");
    dd.show(stdout, 2);
#endif
    dd *= mask2;
#ifdef DEBUG_NOCHANGE
    printf("final dd:\n");
    dd.show(stdout, 2);
#endif

    return;
  } // try
  catch (error e) {
    // cleanup, just in case
    Delete(x);
    Delete(xp);

    // An error occurred, report it...
    if (getParent().StartError(0)) {
      getParent().SendError("Meddly error ");
      getParent().SendError(e.getName());
      getParent().SendError(" in buildNoChange");
      getParent().DoneError();
    }
    // ...and figure out which one
    if (error::INSUFFICIENT_MEMORY == e.getCode()) {
      throw subengine::Out_Of_Memory;
    } else {
      throw subengine::Engine_Failed;
    }
  } // catch
}






void bounded_varoption::checkBounds(const exprman* em)
{
  //
  // Check that all state variables are bounded
  //
  if (!built_ok) return;
  for (int i=0; i<getParent().getNumStateVars(); i++) {
    const model_statevar* sv = getParent().readStateVar(i);
    if (sv->HasBounds()) continue;

    if (built_ok) {
      built_ok = false;
      if (!getParent().StartError(0)) return;
      em->cerr() << "BOUNDED setting requires bounds for state variables.";
      em->newLine();
      em->cerr() << "The following state variables do not have declared bounds:";
      em->newLine();
      em->cerr() << sv->Name();
    } else {
      em->cerr() << ", " << sv->Name();
    }
  } // for i
  if (!built_ok) getParent().DoneError();
}

int bounded_varoption::initDomain(const exprman* em)
{
  //
  // Build the domain
  //
  if (!built_ok) return 0;
  
  const hldsm::partinfo& part = getParent().getPartInfo();
  minterm = new int[part.num_levels+1];
  minprim = new int[part.num_levels+1];
  minterm[0] = 1;
  minprim[0] = 1;
  int maxbound = 0;
  variable** vars = new variable*[part.num_levels+1];
  vars[0] = 0;
  for (int k=part.num_levels; k; k--) {
    int bnd = 1;
    for (int i=part.pointer[k]; i>part.pointer[k-1]; i--) {
      DCASSERT(part.variable[i]);
      int nv = part.variable[i]->NumPossibleValues();
      int newbnd = bnd * nv;
      if (newbnd < 1 || newbnd / nv != bnd) {
        if (getParent().StartError(0)) {
          em->cerr() << "Overflow in size of level " << k << " for MDD";
          getParent().DoneError();
        }
        built_ok = false;
        return 0;
      }
      bnd = newbnd;
    } // for p
    vars[k] = createVariable(bnd, buildVarName(part, k));
    maxbound = MAX(bnd, maxbound);

  } // for k
  if (!ms.createVars(vars, part.num_levels)) {
    built_ok = false;
    return 0;
  }
  return maxbound;
}

void bounded_varoption::initEncoders(int maxbound, const meddly_procgen &pg)
{
  if (!built_ok) return;
  //
  // Initialize MDD forest
  //
  forest* mdd = ms.createForest(
    false, forest::BOOLEAN, forest::MULTI_TERMINAL, 
    pg.buildRSSPolicies()
  );
  DCASSERT(mdd);
  //
  // Initialize MxD forest
  //
  forest* mxd = ms.createForest(
    true, forest::BOOLEAN, forest::MULTI_TERMINAL,
    pg.buildNSFPolicies()
  );
  DCASSERT(mxd);
  //
  // Initialize MTMxD forest
  //
  forest* mtmxd = ms.createForest(
    true, forest::INTEGER, forest::MULTI_TERMINAL,
    pg.buildNSFPolicies()
  );
  DCASSERT(mtmxd);
  //
  // Build encoders
  //
  ms.setMddWrap(new bounded_encoder("MDD", mdd, getParent(), maxbound));
  set_mxd_wrap(new bounded_encoder("MxD", mxd, getParent(), maxbound));
  mtmxd_wrap =  new bounded_encoder("MTMxD", mtmxd, getParent(), maxbound);
}


// **************************************************************************
// *                                                                        *
// *                         substate_encoder class                         *
// *                                                                        *
// **************************************************************************

class substate_encoder : public meddly_encoder {
  substate_colls* colls;
  const hldsm &parent;
public:
  substate_encoder(const char* n, forest* f, const hldsm &p, substate_colls* c);
protected:
  virtual ~substate_encoder();
public:
  virtual bool arePrimedVarsSeparate() const { return false; }
  virtual int getNumDDVars() const { return parent.getPartInfo().num_levels; }
  virtual void buildSymbolicSV(const symbol* sv, bool primed, 
                                expr *f, shared_object* answer);

  virtual void state2minterm(const shared_state* s, int* mt) const;
  virtual void minterm2state(const int* mt, shared_state *s) const;

  virtual meddly_encoder* copyWithDifferentForest(const char* n, forest*) const;
};

// **************************************************************************
// *                                                                        *
// *                        substate_encoder methods                        *
// *                                                                        *
// **************************************************************************

substate_encoder
::substate_encoder(const char* n, forest* f, const hldsm &p, substate_colls* c)
: meddly_encoder(n, f), parent(p)
{
  colls = c;
  DCASSERT(parent.hasPartInfo());
}

substate_encoder::~substate_encoder()
{
  Delete(colls);
}

void substate_encoder
::buildSymbolicSV(const symbol* sv, bool primed, expr* f, shared_object* answer)
{
  throw Failed;
}

void 
substate_encoder::state2minterm(const shared_state* s, int* mt) const
{
  DCASSERT(colls);
  if (0==s || 0==mt) throw Failed;

  for (int k=parent.getPartInfo().num_levels; k; k--) {
    int ssz = s->readSubstateSize(k);
    long sk = colls->addSubstate(k, s->readSubstate(k), ssz);
    if (sk<0) {
      if (-2 == sk) throw Out_Of_Memory;
      throw Failed;
    }
    mt[k] = sk;
    // check overflow
    if (sk != mt[k]) throw Failed;
    // TBD: Should we print an error for that one?
  } // for k
}

void
substate_encoder::minterm2state(const int* mt, shared_state *s) const
{
  DCASSERT(colls);
  if (0==s || 0==mt) throw Failed;

  for (int k=parent.getPartInfo().num_levels; k; k--) {
    int ssz = s->readSubstateSize(k);
    int foo = colls->getSubstate(k, mt[k], s->writeSubstate(k), ssz);
    if (foo<0) throw Failed;
  } // for k
}

meddly_encoder* 
substate_encoder::copyWithDifferentForest(const char* n, forest* nf) const
{
  return new substate_encoder(n, nf, parent, Share(colls));
}

// **************************************************************************
// *                                                                        *
// *                        enabling_subevent  class                        *
// *                                                                        *
// **************************************************************************

// TBD - move this somewhere better
//
class enabling_subevent : public satotf_opname::subevent {
  public:
    // TBD - clean up this constructor!
    // enabling_subevent(named_msg &d, const dsde_hlm &p, substate_colls *c, intset event_deps, expr* chunk, int* v, int nv);
    enabling_subevent(named_msg &d, const dsde_hlm &p, const model_event* Ev, substate_colls *c, intset event_deps, expr* chunk, forest* f, int* v, int nv);
    virtual ~enabling_subevent();

  protected:
    virtual void confirm(satotf_opname::otf_relation &rel, int v, int index);

  private: // helpers
#ifndef USING_MEDDLY_ADD_MINTERM
    // returns true on success
    bool addMinterm(const int* from, const int* to);
#endif

    void exploreEnabling(satotf_opname::otf_relation &rel, int dpth);

    inline bool maybeEnabled() {
#ifdef SHORT_CIRCUIT_ENABLING
      DCASSERT(is_enabled);
      is_enabled->Compute(td);
      if (td.answer->isNormal()) {
        return td.answer->getBool();
      } else if (td.answer->isUnknown()) {
        return true;
      } else {
        // null?  not sure what to do here
        return false;
      }
#else
      return true;
#endif
    }

  private:
    expr* is_enabled;
    int** mt_from;
    int** mt_to;
    int mt_alloc;
    int mt_used;
    int num_levels;
    // some of this stuff could be shared, one per model,
    // at the cost of re-initializing stuff every time we need to explore
    // TBD - option to select between "optimize for speed" and "optimize for memory"
    // (two different classes?  or make this a template <bool> class?)
    traverse_data td;
    shared_state* tdcurr;
    int* from_minterm;
    int* to_minterm;

    // used by exploreEnabling
    int changed_k;
    int new_index;

    substate_colls* colls;

    // used only for debug info?
    const model_event* E;

    // TBD - this should be static or accessed via a parent class
    named_msg &debug;
};

// **************************************************************************
// *                       enabling_subevent  methods                       *
// **************************************************************************

// enabling_subevent::enabling_subevent(named_msg &d, const dsde_hlm &p, substate_colls* c, intset event_deps, expr* chunk, int* v, int nv)
enabling_subevent::enabling_subevent(named_msg &d, const dsde_hlm &p, const model_event* Ev, substate_colls* c, intset event_deps, expr* chunk, forest* f, int* v, int nv)
 : satotf_opname::subevent(f, v, nv), td(traverse_data::Compute), debug(d)
{
  E = Ev;
  is_enabled = chunk;
  colls = c;

  mt_from = 0;
  mt_to = 0;
  mt_alloc = mt_used = 0;
  num_levels = p.getPartInfo().num_levels;

  td.answer = new result;
  tdcurr = new shared_state(&p);
  td.current_state = tdcurr;
  from_minterm = new int[1+num_levels];
  to_minterm = new int[1+num_levels];

  from_minterm[0] = DONT_CARE;
  to_minterm[0] = DONT_CHANGE;
  for (int k=1; k<=num_levels; k++) {
    from_minterm[k] = DONT_CARE;
    if (event_deps.contains(k)) {
      to_minterm[k] = DONT_CARE;
    } else {
      to_minterm[k] = DONT_CHANGE;
    }
    tdcurr->set_substate_unknown(k);
  }
}

enabling_subevent::~enabling_subevent()
{
  Delete(is_enabled);
  for (int i=0; i<mt_used; i++) {
    delete[] mt_from[i];
    delete[] mt_to[i];
  }
  free(mt_from);
  free(mt_to);

  delete td.answer;
  Delete(tdcurr);
  delete[] from_minterm;
  delete[] to_minterm;
}

void enabling_subevent::confirm(satotf_opname::otf_relation &rel, int k, int index)
{
  DCASSERT(E);
  if (debug.startReport()) {
    debug.report() << "confirming level " << k << " index " << index;
    debug.report() << " event " << E->Name() << " enabling ";
    is_enabled->Print(debug.report(), 0);
    debug.report() << "\n";
    debug.stopIO();
  }

  changed_k = k;
  new_index = index;

  // Explicitly explore new states and discover minterms to add

  exploreEnabling(rel, getNumVars());

#ifndef USING_MEDDLY_ADD_MINTERM
  if (0==mt_used) return;

  // Add those minterms
  dd_edge add_to_root(getForest());
  DCASSERT(mt_from);
  DCASSERT(mt_to);

  if (debug.startReport()) {
    for (int i = 0; i < mt_used; i++) {
      debug.report() << "\nEnabling\n";
      debug.report() << "\nfrom[" << i << "]: [";
      debug.report().PutArray(mt_from[i]+1, num_levels);
      debug.report() << "]\nto[" << i << "]: [";
      debug.report().PutArray(mt_to[i]+1, num_levels);
      debug.report() << "]\n";
      debug.report() << "\n\n";
    }
    debug.stopIO();
  }

  getForest()->createEdge(mt_from, mt_to, mt_used, add_to_root);
  mt_used = 0;

  add_to_root += getRoot();
  setRoot(add_to_root);

  if (debug.startReport()) {
    debug.report() << "confirmed  level " << k << " index " << index;
    debug.report() << " event " << E->Name() << " enabling ";
    is_enabled->Print(debug.report(), 0);
    debug.newLine();
    debug.report() << "New root: " << add_to_root.getNode() << "\n";
    debug.stopIO();
  }
#endif
}

#ifndef USING_MEDDLY_ADD_MINTERM
bool enabling_subevent::addMinterm(const int* from, const int* to)
{
  if (mt_used >= mt_alloc) {
    int old_alloc = mt_alloc;
    if (0==mt_alloc) {
      mt_alloc = 8;
      mt_from = (int**) malloc(mt_alloc * sizeof(int**));
      mt_to = (int**) malloc(mt_alloc * sizeof(int**));
    } else {
      mt_alloc = MIN(2*mt_alloc, 256 + mt_alloc);
      mt_from = (int**) realloc(mt_from, mt_alloc * sizeof(int**));
      mt_to = (int**) realloc(mt_to, mt_alloc * sizeof(int**));
    }
    if (0==mt_from || 0==mt_to) return false; // malloc or realloc failed
    for (int i=old_alloc; i<mt_alloc; i++) {
      mt_from[i] = 0;
      mt_to[i] = 0;
    }
  }
  if (0==mt_from[mt_used]) {
    mt_from[mt_used] = new int[1+num_levels];
    DCASSERT(0==mt_to[mt_used]);
    mt_to[mt_used] = new int[1+num_levels];
  }
  memcpy(mt_from[mt_used], from, (1+num_levels) * sizeof(int));
  memcpy(mt_to[mt_used], to, (1+num_levels) * sizeof(int));
  mt_used++;
  return true;
}
#endif

void enabling_subevent::exploreEnabling(satotf_opname::otf_relation &rel, int dpth)
{
  //
  // Are we at the bottom?
  //
  if (0==dpth) {
    bool start_d = debug.startReport();
    if (start_d) {
      debug.report() << "enabled?\n\tstate ";
      tdcurr->Print(debug.report(), 0);
      debug.report() << "\n\tfrom minterm [";
      debug.report().PutArray(from_minterm+1, num_levels);
      debug.report() << "]\n\t";
      is_enabled->Print(debug.report(), 0);
      debug.report() << " : ";
    }
#ifdef SHORT_CIRCUIT_ENABLING
    td.answer->setBool(true);
#else
    DCASSERT(is_enabled);
    is_enabled->Compute(td);
#endif
    if (start_d) {
      if (td.answer->isNormal()) {
        if (td.answer->getBool())
          debug.report() << "true";
        else
          debug.report() << "false";
      } else if (td.answer->isUnknown())
        debug.report() << "?";
      else
        debug.report() << "null";
      debug.report() << "\n";
      //debug.stopIO();
    }

    DCASSERT(td.answer->isNormal());
    if (false == td.answer->getBool()) return;

    //
    // next state -> minterm
    //
    /*
        // ASM - this should already be set!

    for (int dd=0; dd<getNumVars(); dd++) {
      int kk = getVars()[dd];
      to_minterm[kk] = DONT_CARE;
    }
    */

    //
    // More debug info
    //
    if (debug.startReport()) {
      debug.report() << "enabled\n\tstate ";
      tdcurr->Print(debug.report(), 0);
      debug.report() << "\n\tto minterm [";
      debug.report().PutArray(to_minterm+1, num_levels);
      debug.report() << "]\n";
      debug.stopIO();
    }

    addMinterm(from_minterm, to_minterm);

    return;
  }

  //
  // Are we at the level being confirmed?
  //
  const int k = getVars()[dpth-1];
  if (k == changed_k) {
      tdcurr->set_substate_known(changed_k);
      int ssz = tdcurr->readSubstateSize(changed_k);
      int foo = colls->getSubstate(changed_k, new_index, tdcurr->writeSubstate(changed_k), ssz);
      if (foo<0) throw subengine::Engine_Failed;

      // 
      // check expression and short-circuit if definitely false 
      // tbd - what about definitely true?
      //
      
      if (maybeEnabled()) {
        from_minterm[changed_k] = new_index;
        exploreEnabling(rel, dpth-1); 
      }

      from_minterm[changed_k] = DONT_CARE;
      tdcurr->set_substate_unknown(changed_k);
      return;
  }

  //
  // Regular level - explore all confirmed
  //
  tdcurr->set_substate_known(k);
  const int localsize = rel.getRelForest()->getLevelSize(k);
  for (int i = 0; i<localsize; i++) {
      if (!rel.isConfirmed(k, i)) continue; // skip unconfirmed

      int ssz = tdcurr->readSubstateSize(k);
      int foo = colls->getSubstate(k, i, tdcurr->writeSubstate(k), ssz);
      if (foo<0) throw subengine::Engine_Failed;

      // 
      // check expression and short-circuit if definitely false
      //
      
      if (maybeEnabled()) {
        from_minterm[k] = i;
        exploreEnabling(rel, dpth-1);
      }
  } // for i
  from_minterm[k] = DONT_CARE;
  tdcurr->set_substate_unknown(k);
}


// **************************************************************************
// *                                                                        *
// *                         firing_subevent  class                         *
// *                                                                        *
// **************************************************************************

// TBD - move this somewhere better
//
class firing_subevent : public satotf_opname::subevent {
  public:
    // TBD - clean up this constructor!
    firing_subevent(named_msg &d, const dsde_hlm &p, const model_event* Ev, substate_colls *c, intset event_deps, expr* chunk, forest* f, int* v, int nv);
    virtual ~firing_subevent();

  protected:
    virtual void confirm(satotf_opname::otf_relation &rel, int v, int index);

  private: // helpers
#ifndef USING_MEDDLY_ADD_MINTERM
    // returns true on success
    bool addMinterm(const int* from, const int* to);
#endif

    void exploreFiring(satotf_opname::otf_relation &rel, int dpth);

  private:
    expr* fire_expr;
    int** mt_from;
    int** mt_to;
    int mt_alloc;
    int mt_used;
    int num_levels;
    // some of this stuff could be shared, one per model,
    // at the cost of re-initializing stuff every time we need to explore
    // TBD - option to select between "optimize for speed" and "optimize for memory"
    // (two different classes?  or make this a template <bool> class?)
    traverse_data td;
    shared_state* tdcurr;
    shared_state* tdnext;
    int* from_minterm;
    int* to_minterm;

    // used by exploreFiring
    int changed_k;
    int new_index;

    substate_colls* colls;

    // used only for debug info?
    const model_event* E;

    // TBD - this should be static or accessed via a parent class
    named_msg &debug;
};

// **************************************************************************
// *                        firing_subevent  methods                        *
// **************************************************************************

firing_subevent::firing_subevent(named_msg &d, const dsde_hlm &p, const model_event* Ev, substate_colls* c, intset event_deps, expr* chunk, forest* f, int* v, int nv)
 : satotf_opname::subevent(f, v, nv), td(traverse_data::Compute), debug(d)
{
  E = Ev;
  fire_expr = chunk;
  colls = c;

  mt_from = 0;
  mt_to = 0;
  mt_alloc = mt_used = 0;
  num_levels = p.getPartInfo().num_levels;

  td.answer = new result;
  tdcurr = new shared_state(&p);
  tdnext = new shared_state(&p);
  td.current_state = tdcurr;
  td.next_state = tdnext;
  from_minterm = new int[1+num_levels];
  to_minterm = new int[1+num_levels];

  from_minterm[0] = DONT_CARE;
  to_minterm[0] = DONT_CARE;
  for (int k=1; k<=num_levels; k++) {
    from_minterm[k] = DONT_CARE;
    if (event_deps.contains(k)) {
      to_minterm[k] = DONT_CARE;
    } else {
      to_minterm[k] = DONT_CHANGE;
    }
    tdcurr->set_substate_unknown(k);
    tdnext->set_substate_unknown(k);
  }
}

firing_subevent::~firing_subevent()
{
  Delete(fire_expr);
  for (int i=0; i<mt_used; i++) {
    delete[] mt_from[i];
    delete[] mt_to[i];
  }
  free(mt_from);
  free(mt_to);

  delete td.answer;
  Delete(tdcurr);
  Delete(tdnext);
  delete[] from_minterm;
  delete[] to_minterm;
}

void firing_subevent::confirm(satotf_opname::otf_relation &rel, int k, int index)
{
  DCASSERT(E);
  if (debug.startReport()) {
    debug.report() << "confirming level " << k << " index " << index;
    debug.report() << " event " << E->Name() << " firing ";
    fire_expr->Print(debug.report(), 0);
    debug.report() << "\n";
    debug.stopIO();
  }

  changed_k = k;
  new_index = index;

  // Explicitly explore new states and discover minterms to add

  exploreFiring(rel, getNumVars());

#ifndef USING_MEDDLY_ADD_MINTERM
  if (0==mt_used) return;

  if (debug.startReport()) {
    for (int i = 0; i < mt_used; i++) {
      debug.report() << "\nFiring:\n";
      debug.report() << "\nfrom[" << i << "]: [";
      debug.report().PutArray(mt_from[i]+1, num_levels);
      debug.report() << "]\nto[" << i << "]: [";
      debug.report().PutArray(mt_to[i]+1, num_levels);
      debug.report() << "]\n";
      debug.report() << "\n\n";
    }
    debug.stopIO();
  }

  // Add those minterms
  dd_edge add_to_root(getForest());
  getForest()->createEdge(mt_from, mt_to, mt_used, add_to_root);
  mt_used = 0;

  add_to_root += getRoot();
  setRoot(add_to_root);

  if (debug.startReport()) {
    debug.report() << "confirmed  level " << k << " index " << index;
    debug.report() << " event " << E->Name() << " firing ";
    fire_expr->Print(debug.report(), 0);
    debug.newLine();
    debug.report() << "New root: " << add_to_root.getNode() << "\n";
    debug.stopIO();
  }
#endif
}

#ifndef USING_MEDDLY_ADD_MINTERM
bool firing_subevent::addMinterm(const int* from, const int* to)
{
  if (mt_used >= mt_alloc) {
    int old_alloc = mt_alloc;
    if (0==mt_alloc) {
      mt_alloc = 8;
      mt_from = (int**) malloc(mt_alloc * sizeof(int**));
      mt_to = (int**) malloc(mt_alloc * sizeof(int**));
    } else {
      mt_alloc = MIN(2*mt_alloc, 256 + mt_alloc);
      mt_from = (int**) realloc(mt_from, mt_alloc * sizeof(int**));
      mt_to = (int**) realloc(mt_to, mt_alloc * sizeof(int**));
    }
    if (0==mt_from || 0==mt_to) return false; // malloc or realloc failed
    for (int i=old_alloc; i<mt_alloc; i++) {
      mt_from[i] = 0;
      mt_to[i] = 0;
    }
  }
  if (0==mt_from[mt_used]) {
    mt_from[mt_used] = new int[1+num_levels];
  }
  if (0==mt_to[mt_used]) {
    mt_to[mt_used] = new int[1+num_levels];
  }
  memcpy(mt_from[mt_used], from, (1+num_levels) * sizeof(int));
  memcpy(mt_to[mt_used], to, (1+num_levels) * sizeof(int));
  mt_used++;
  return true;
}
#endif

void firing_subevent::exploreFiring(satotf_opname::otf_relation &rel, int dpth)
{
  //
  // Are we at the bottom?
  //
  if (0==dpth) {
    bool start_d = debug.startReport();
    if (start_d) {
      debug.report() << "firing?\n\tfrom state ";
      tdcurr->Print(debug.report(), 0);
      debug.report() << "\n\tfrom minterm [";
      debug.report().PutArray(from_minterm+1, num_levels);
      debug.report() << "]\n\t";
      fire_expr->Print(debug.report(), 0);
      debug.report() << " : ";
    }
    DCASSERT(fire_expr);
    fire_expr->Compute(td);
    if (start_d) {
      if (td.answer->isNormal()) 
        debug.report() << "ok";
      else if (td.answer->isUnknown())
        debug.report() << "?";
      else if (td.answer->isOutOfBounds())
        debug.report() << "out of bounds";
      else
        debug.report() << "null";
      debug.report() << "\n";
      debug.stopIO();
    }

    if (!td.answer->isNormal()) return; // not enabled after all

    //
    // next state -> minterm
    for (int dd=0; dd<getNumVars(); dd++) {
      int kk = getVars()[dd];
      to_minterm[kk] = colls->addSubstate(kk, 
          tdnext->readSubstate(kk), tdnext->readSubstateSize(kk)
      );
    
      if (to_minterm[kk]>=0) continue;
      if (-2==to_minterm[kk]) throw subengine::Out_Of_Memory;
      throw subengine::Engine_Failed;
    }

    //
    // More debug info
    //
    if (debug.startReport()) {
      debug.report() << "firing\n\tto state ";
      tdnext->Print(debug.report(), 0);
      debug.report() << "\n\tto minterm [";
      debug.report().PutArray(to_minterm+1, num_levels);
      debug.report() << "]\n";
      debug.stopIO();
    }

    // add minterm to queue
    //
    if (!addMinterm(from_minterm, to_minterm)) 
      throw subengine::Out_Of_Memory;

    return;
  }

  //
  // Are we at the level being confirmed?
  //
  const int k = getVars()[dpth-1];
  if (k == changed_k) {
      tdcurr->set_substate_known(changed_k);
      tdnext->set_substate_known(changed_k);
      int ssz = tdcurr->readSubstateSize(changed_k);
      int foo = colls->getSubstate(changed_k, new_index, tdcurr->writeSubstate(changed_k), ssz);
      if (foo<0) throw subengine::Engine_Failed;
      colls->getSubstate(changed_k, new_index, tdnext->writeSubstate(changed_k), ssz);

      // 
      // tbd - short circuit?
      //
      
      from_minterm[changed_k] = new_index;
      exploreFiring(rel, dpth-1); 

      from_minterm[changed_k] = DONT_CARE;
      to_minterm[changed_k] = DONT_CARE;
      tdcurr->set_substate_unknown(changed_k);
      tdnext->set_substate_unknown(changed_k);
      return;
  }

  //
  // Regular level - explore all confirmed
  //
  tdcurr->set_substate_known(k);
  tdnext->set_substate_known(k);
  const int localsize = rel.getRelForest()->getLevelSize(k);
  for (int i = 0; i<localsize; i++) {
      if (!rel.isConfirmed(k, i)) continue; // skip unconfirmed

      int ssz = tdcurr->readSubstateSize(k);
      int foo = colls->getSubstate(k, i, tdcurr->writeSubstate(k), ssz);
      if (foo<0) throw subengine::Engine_Failed;
      colls->getSubstate(k, i, tdnext->writeSubstate(k), ssz);

      // 
      // tbd - short circuit?
      //
      
      from_minterm[k] = i;
      exploreFiring(rel, dpth-1);
  } // for i
  from_minterm[k] = DONT_CARE;
  to_minterm[k] = DONT_CARE;
  tdcurr->set_substate_unknown(k);
  tdnext->set_substate_unknown(k);
}

// **************************************************************************
// *                                                                        *
// *                        substate_varoption class                        *
// *                                                                        *
// **************************************************************************

// Abstract base class
class substate_varoption : public meddly_varoption {
protected:
  struct expr_node {
    expr* term;
    expr_node* next;
  };
  struct deplist {
    expr_node* termlist;
    deplist* next;
    // mxd 
    // dd_edge* dd;
    // explicit storage of minterms
  private:
    intset level_deps;
    int** mt_from;
    int** mt_to;
    int mt_alloc;
    int mt_used;
    int depth;
  public:
    deplist(const intset &x, expr_node* tl, deplist* n, int d);
    ~deplist();
    // returns true on success
    bool addMinterm(const int* from);
    // returns true on success
    bool addMinterm(const int* from, const int* to);
    // show minterms
    void showMinterms(OutputStream &s, int pad);
    void showMintermPairs(OutputStream &s, int pad);

    inline int getLevelAbove(int k) const {
      return level_deps.getSmallestAfter(k);
    }
    inline bool sameDeps(const intset& deps) const {
      return level_deps == deps;
    }
    inline void addToDeps(intset& deps) const {
      deps += level_deps;
    }
    inline int countDeps() const {
      return level_deps.cardinality();
    }

    /**
        Given a list of levels, build the intersection
        with the set of levels that we depend on.
        This is unsafe - no bound checking on the arrays - so make sure
        the arrays are large enough :^)

          @param  levels_in   Zero-terminated array of levels, as input.
          @param  levels_out  Resulting list stored here, as a
                              zero-terminated array of levels.
    */
    inline void intersectLevels(const int* levels_in, int* levels_out) const {
      while (*levels_in) {
        if (level_deps.contains(*levels_in)) {
          *levels_out = *levels_in;
          levels_out++;
        }
        levels_in++;
      }
      *levels_out = 0;
    }
  private:
    void expandLists();
  };
protected:
  deplist** enable_deps;
  deplist** fire_deps;
  substate_colls* colls;
  intset* confirmed;    // substates that are confirmed
  intset* toBeExplored; // substates to explore next round
  int num_levels;
private:
  traverse_data td;
  shared_state* tdcurr;
  shared_state* tdnext;
  int* from_minterm;
  int* to_minterm;
  int* tmpLevels;   // array of size num_levels+1
public:
  substate_varoption(meddly_reachset &x, const dsde_hlm &p, 
    const exprman* em, const meddly_procgen &pg);
  virtual ~substate_varoption();
  virtual void initializeVars();
  virtual void initializeEvents(named_msg &d);
  virtual void reportStats(DisplayStream &out) const;

  //
  // TBD - for now
  //

  virtual satotf_opname::otf_relation* buildNSF_OTF(named_msg &debug);

private:
  void initDomain(const exprman* em);
  void initEncoders(const meddly_procgen &pg);
  static deplist* getExprDeps(expr *x, int numlevels);
  static void clearList(deplist* &L);
  inline void enlarge(intset &set, int N) {
    int oldsz = set.getSize();
    if (oldsz >= N) return;
    int newsz = oldsz;
    while (newsz < N) newsz += 256;
    set.resetSize(newsz);
    set.removeRange(oldsz, newsz-1);
  }

  /**
      Add minterms for enabling expression.
        @param  d         Debugging stream
        @param  dl        Chunk of function.
        @param  k         Current level
        @param  changed   0, if we have already seen an unexplored local.
                          Otherwise, zero-terminated list of levels with
                          unexplored locals to pull from.
  */
  void exploreEnabling(named_msg &d, deplist &dl, int k, const int* changed);
  

  /**
      Add minterms for next-state expression.
        @param  d         Debugging stream
        @param  dl        Chunk of function.
        @param  k         Current level
        @param  changed   0, if we have already seen an unexplored local.
                          Otherwise, zero-terminated list of levels with
                          unexplored locals to pull from.
  */
  void exploreNextstate(named_msg &d, deplist &dl, int k, const int* changed);
  
protected:
  /**
      Update all enabling and next-state functions for the given list of 
      modified (i.e., new local states to explore) levels.
      The unexplored locals at those levels are then marked as explored.
        @param  d         Debugging stream
        @param  levels    Zero-terminated list of levels, from
                          lowest to highest.
  */
  void updateLevels(named_msg &d, const int* levels);

  /**
      Build the list of levels that have confirmed, unexplored locals.
        @param  levels    Array of dimension num_levels+1 or larger.
                          On output - a zero terminated list of levels.
  */
  inline void buildLevelsToExplore(int* levels) {
    for (int k=1; k<=num_levels; k++) {
      int first = toBeExplored[k].getSmallestAfter(-1);
      if (first<0) continue;
      *levels = k;
      levels++;
    }
    *levels = 0;
  }

#ifdef SHOW_SUBSTATES
  void show_substates(OutputStream &s);
#endif
};

// **************************************************************************
// *                                                                        *
// *                  substate_varoption::deplist  methods                  *
// *                                                                        *
// **************************************************************************

substate_varoption::deplist
::deplist(const intset &x, expr_node* tl, deplist* n, int d) : level_deps(x) 
{ 
  termlist = tl;
  // dd = 0;
  next = n;
  mt_from = mt_to = 0;
  mt_alloc = mt_used = 0;
  depth = d;
}

substate_varoption::deplist::~deplist()
{
  for (int i=0; i<mt_used; i++) {
    delete[] mt_from[i];
    delete[] mt_to[i];
  }
  free(mt_from);
  free(mt_to);
}

bool substate_varoption::deplist::addMinterm(const int* from)
{
  if (mt_used >= mt_alloc) {
    expandLists();
  }
  if (0==mt_from) return false; // malloc or realloc failed
  if (0==mt_from[mt_used]) {
    mt_from[mt_used] = new int[depth];
  }
  memcpy(mt_from[mt_used], from, depth * sizeof(int));
  if (0==mt_to[mt_used]) {
    mt_to[mt_used] = new int[depth];
  }
  for (int i=0; i<depth; i++) {
    mt_to[mt_used][i] = DONT_CARE;
  }
  mt_used++;
  return true;
}

bool substate_varoption::deplist::addMinterm(const int* from, const int* to)
{
  if (mt_used >= mt_alloc) {
    expandLists();
  }
  if (0==mt_from || 0==mt_to) return false; // malloc or realloc failed
  if (0==mt_from[mt_used]) {
    mt_from[mt_used] = new int[depth];
  }
  memcpy(mt_from[mt_used], from, depth * sizeof(int));
  if (0==mt_to[mt_used]) {
    mt_to[mt_used] = new int[depth];
  }
  memcpy(mt_to[mt_used], to, depth * sizeof(int));
  mt_used++;
  return true;
}

void substate_varoption::deplist::showMinterms(OutputStream &s, int pad)
{
  for (int i=0; i<mt_used; i++) {
    s.Pad(' ', pad);
    s.Put('[');
    s.PutArray(mt_from[i]+1, depth-1);
    s << "]\n";
  }
  if (next) {
    s.Pad(' ', pad);
    s << "(synch)\n";
    next->showMinterms(s, pad);
  }
}

void substate_varoption::deplist::showMintermPairs(OutputStream &s, int pad)
{
  for (int i=0; i<mt_used; i++) {
    s.Pad(' ', pad);
    s.Put('[');
    s.PutArray(mt_from[i]+1, depth-1);
    s << "], [";
    s.PutArray(mt_to[i]+1, depth-1);
    s << "]\n";
  }
  if (next) {
    s.Pad(' ', pad);
    s << "(synch)\n";
    next->showMintermPairs(s, pad);
  }
}

void substate_varoption::deplist::expandLists()
{
  // resize arrays...
  int old_alloc = mt_alloc;
  if (0==mt_alloc) {
    mt_alloc = 8;
    mt_from = (int**) malloc(mt_alloc * sizeof(int**));
    mt_to = (int**) malloc(mt_alloc * sizeof(int**));
  } else {
    mt_alloc = MIN(2*mt_alloc, 256 + mt_alloc);
    mt_from = (int**) realloc(mt_from, mt_alloc * sizeof(int**));
    mt_to = (int**) realloc(mt_to, mt_alloc * sizeof(int**));
  }
  for (int i=old_alloc; i<mt_alloc; i++) {
    mt_from[i] = 0;
    mt_to[i] = 0;
  }
}

// **************************************************************************
// *                                                                        *
// *                       substate_varoption methods                       *
// *                                                                        *
// **************************************************************************

substate_varoption
::substate_varoption(meddly_reachset &x, const dsde_hlm &p, 
  const exprman* em, const meddly_procgen &pg) 
: meddly_varoption(x, p), td(traverse_data::Compute)
{
  colls = 0;
  enable_deps = 0;
  fire_deps = 0;
  confirmed = 0;
  toBeExplored = 0;
  td.answer = new result;
  tdcurr = new shared_state(&p);
  tdnext = new shared_state(&p);
  td.current_state = tdcurr;
  td.next_state = tdnext;
  from_minterm = 0;
  to_minterm = 0;
  initDomain(em);
  initEncoders(pg);
  tmpLevels = 0;
}

substate_varoption::~substate_varoption()
{
  Delete(colls);
  delete[] confirmed;
  delete[] toBeExplored;
  delete[] from_minterm;
  delete[] to_minterm;
  Delete(tdcurr);
  Delete(tdnext);
  delete td.answer;
  delete[] tmpLevels;
}

void substate_varoption::initializeVars()
{
  meddly_varoption::initializeVars();
  // confirm the initial substates
  for (int k=num_levels; k; k--) {
    int N = colls->getMaxIndex(k);
    enlarge(confirmed[k], N);
    confirmed[k].addRange(0, N-1);
    enlarge(toBeExplored[k], N);
    toBeExplored[k].addRange(0, N-1);
  } // for k
  // make scratch space
  tmpLevels = new int[num_levels+1];
}
  
void 
substate_varoption::initializeEvents(named_msg &d)
{
  if (d.startReport()) {
    d.report() << "preprocessing event expressions\n";
    d.stopIO();
  }
  enable_deps = new deplist*[getParent().getNumEvents()];
  fire_deps = new deplist*[getParent().getNumEvents()];
  for (int i=0; i<getParent().getNumEvents(); i++) {
    const model_event* e = getParent().readEvent(i);
    enable_deps[i] = getExprDeps(e->getEnabling(), num_levels);
    fire_deps[i] = getExprDeps(e->getNextstate(), num_levels);
    DCASSERT(enable_deps[i]);
    DCASSERT(enable_deps[i]->termlist);
    DCASSERT(fire_deps[i]);
    DCASSERT(fire_deps[i]->termlist);
  } // for i

  if (!d.startReport()) return;
  d.report() << "done preprocessing event expressions\n";

  for (int i=0; i<getParent().getNumEvents(); i++) {

    d.report() << "\t" << getParent().readEvent(i)->Name() << " enabling:\n";
    for (deplist *DL = enable_deps[i]; DL; DL=DL->next) {
      d.report() << "\t\tlevels ";
      int k = DL->getLevelAbove(0);
      for ( ; k>0; k=DL->getLevelAbove(k)) {
        d.report() << k << ", ";
      } // for k
      d.report() << "\n\t\t";
      for (expr_node* term = DL->termlist; term; term=term->next) {
        term->term->Print(d.report(), 0);
        d.report() << ", ";
      } // for term
      d.report() << "\n";
    } // for DL

    d.report() << "\t" << getParent().readEvent(i)->Name() << " firing:\n";
    for (deplist *DL = fire_deps[i]; DL; DL=DL->next) {
      d.report() << "\t\tlevels ";
      int k = DL->getLevelAbove(0);
      for ( ; k>0; k=DL->getLevelAbove(k)) {
        d.report() << k << ", ";
      } // for k
      d.report() << "\n\t\t";
      for (expr_node* term = DL->termlist; term; term=term->next) {
        term->term->Print(d.report(), 0);
        d.report() << ", ";
      } // for term
      d.report() << "\n";
    } // for DL
  } // for i

  d.stopIO();
}

void substate_varoption::reportStats(DisplayStream &out) const
{
  meddly_varoption::reportStats(out);
  DCASSERT(colls);
  colls->Report(out);
}


satotf_opname::otf_relation* substate_varoption::buildNSF_OTF(named_msg &debug)
{
  using namespace MEDDLY;

  exprman* em = getExpressionManager();
  DCASSERT(em);

  satotf_opname::event** otf_events = new
    satotf_opname::event* [getParent().getNumEvents()];

  //
  // For each event
  //
  intset depends(num_levels+1);
  for (int i=0; i<getParent().getNumEvents(); i++) {
    //
    // union the enabling and firing dependencies to get all levels
    // that this event depends on

    depends.removeAll();

    int num_enabling = 0;
    for (deplist* ptr = enable_deps[i]; ptr; ptr=ptr->next, num_enabling++) {
      ptr->addToDeps(depends);
    };

    int num_firing = 0;
    for (deplist* ptr = fire_deps[i]; ptr; ptr=ptr->next, num_firing++) {
      ptr->addToDeps(depends);
    }

    DCASSERT(num_enabling + num_firing > 0);

    //
    // Build subevents
    //

    satotf_opname::subevent** subevents = new 
      satotf_opname::subevent* [num_enabling + num_firing];
    int se = 0;

    //
    // Build firing subevents
    //
    for (deplist* ptr = fire_deps[i]; ptr; ptr=ptr->next, se++) {
      //
      // build firing expression from list
      //
      int length = 0;
      for (expr_node* t = ptr->termlist; t; t=t->next) {
        length++;
      }
      DCASSERT(length>0);
      expr* chunk = 0;
      if (1==length) {
        //
        // awesomesauce
        //
        chunk = Share(ptr->termlist->term);
      } else {
        //
        // Build conjunction
        //
        expr** terms = new expr*[length];
        int ti = 0;
        for (expr_node* t = ptr->termlist; t; t=t->next, ti++) {
          terms[ti] = Share(t->term);
        }
        chunk = em->makeAssocOp(0, -1, exprman::aop_semi, terms, 0, length);
      }

      // build list of variables this piece depends on
      int nv = ptr->countDeps();
      int* v = new int[nv];
      int k = 0;
      for (int vi = 0; vi<nv; vi++) {
        k = ptr->getLevelAbove(k);
        DCASSERT(k>0);
        v[vi] = k;
      }

      // Ok, build the enabling subevent
      const model_event* e = getParent().readEvent(i);
      subevents[se] = new firing_subevent(debug, getParent(), e, colls, depends, chunk, get_mxd_forest(), v, nv);
    }

    //
    // Build enabling subevents
    //
    for (deplist* ptr = enable_deps[i]; ptr; ptr=ptr->next, se++) {
      //
      // build enabling expression from list
      //
      int length = 0;
      for (expr_node* t = ptr->termlist; t; t=t->next) {
        length++;
      }
      DCASSERT(length>0);
      expr* chunk = 0;
      if (1==length) {
        //
        // awesomesauce
        //
        chunk = Share(ptr->termlist->term);
      } else {
        //
        // Build conjunction
        //
        expr** terms = new expr*[length];
        int ti = 0;
        for (expr_node* t = ptr->termlist; t; t=t->next, ti++) {
          terms[ti] = Share(t->term);
        }
        chunk = em->makeAssocOp(0, -1, exprman::aop_and, terms, 0, length);
      }

      // build list of variables this piece depends on
      int nv = ptr->countDeps();
      int* v = new int[nv];
      int k = 0;
      for (int vi = 0; vi<nv; vi++) {
        k = ptr->getLevelAbove(k);
        DCASSERT(k>0);
        v[vi] = k;
      }

      // Ok, build the enabling subevent
      const model_event* e = getParent().readEvent(i);
      subevents[se] = new enabling_subevent(debug, getParent(), e, colls, depends, chunk, get_mxd_forest(), v, nv);
    }

    
    //
    // Pull these together into the event
    //
    otf_events[i] = new satotf_opname::event(subevents, num_enabling + num_firing);
  } // for i

  //
  // Build overall otf relation
  //
  return new satotf_opname::otf_relation(
    ms.getMddForest(), get_mxd_forest(), ms.getMddForest(), 
    otf_events, getParent().getNumEvents()
  );
}


void substate_varoption::initDomain(const exprman* em)
{
  //
  // Build the domain
  //
  if (!built_ok) return;
  
  const hldsm::partinfo& part = getParent().getPartInfo();
  num_levels = part.num_levels;
  variable** vars = new variable*[num_levels+1];
  vars[0] = 0;
  for (int k=num_levels; k; k--) {
    vars[k] = createVariable(1, buildVarName(part, k));
  } // for k
  if (!ms.createVars(vars, num_levels)) {
    built_ok = false;
    return;
  }
  
  from_minterm = new int[num_levels+1];
  to_minterm = new int[num_levels+1];
  from_minterm[0] = DONT_CARE;
  to_minterm[0] = DONT_CHANGE;
  for (int k=1; k<=num_levels; k++) {
    from_minterm[k] = DONT_CARE;
    to_minterm[k] = DONT_CHANGE;
    tdcurr->set_substate_unknown(k);
    tdnext->set_substate_unknown(k);
  }

  //
  // Initialize substate collections
  //
  const exp_state_lib* esl = InitExplicitStateStorage(0);
  DCASSERT(esl);
  colls = esl->createSubstateDBs(num_levels, false);
  DCASSERT(colls);

  //
  // Keep track of which substates are explored, confirmed
  //
  confirmed = new intset[num_levels+1];
  toBeExplored = new intset[num_levels+1];
}

void substate_varoption::initEncoders(const meddly_procgen &pg)
{
  if (!built_ok) return;

  //
  // Initialize MDD forest
  //
  forest* mdd = ms.createForest(
    false, forest::BOOLEAN, forest::MULTI_TERMINAL, 
    pg.buildRSSPolicies()
  );
  DCASSERT(mdd);
  //
  // Initialize MxD forest
  //
  forest* mxd = ms.createForest(
    true, forest::BOOLEAN, forest::MULTI_TERMINAL, 
    pg.buildNSFPolicies()
  );
  DCASSERT(mxd);

  //
  // Build encoders
  //
  ms.setMddWrap(new substate_encoder("MDD", mdd, getParent(), Share(colls)));
  set_mxd_wrap(new substate_encoder("MxD", mxd, getParent(), Share(colls)));
}

substate_varoption::deplist*
substate_varoption::getExprDeps(expr* x, int numlevels)
{
  if (0==x) return 0;

  // first, build list of terms in product
  static List <expr> PL;
  PL.Clear();
  x->BuildExprList(traverse_data::GetProducts, 0, &PL);
  expr_node* termlist = 0;
  for (int i=0; i<PL.Length(); i++) {
    expr_node* link = new expr_node;
    link->term = PL.Item(i);
    link->next = termlist;
    termlist = link;
  } // for i

  // now, merge any with the same level dependencies 
  intset deps(numlevels+1);
  static List <symbol> SL;
  deplist* DL = 0;
  while (termlist) {
    expr_node* curr = termlist;
    termlist = curr->next;
    
    // get level dependencies
    deps.removeAll();
    SL.Clear();
    curr->term->BuildSymbolList(traverse_data::GetSymbols, 0, &SL);
    for (int i=0; i<SL.Length(); i++) {
      model_statevar* mv = dynamic_cast <model_statevar*> (SL.Item(i));
      if (0==mv) continue;
      CHECK_RANGE(1, mv->GetPart(), numlevels+1);
      deps.addElement(mv->GetPart());
    } // for i
    
    // check deplist for any with same dependencies
    deplist* find;
    for (find = DL; find; find=find->next) {
      if (find->sameDeps(deps)) break;
    } // for find

    if (find) {
      // add current term to this group
      curr->next = find->termlist;
      find->termlist = curr;
    } else {
      // no match; start a new group
      find = new deplist(deps, curr, DL, numlevels+1);
      curr->next = 0;
      DL = find;
    }

  } // while termlist

  return DL;
}

void
substate_varoption::clearList(deplist* &L)
{
  while (L) {
    while (L->termlist) {
      expr_node* tl = L->termlist;
      L->termlist = tl->next;
      delete tl;
    }
    deplist *tmp = L;
    L = L->next;
    delete tmp;
  }
  L=0;
}


void substate_varoption
::exploreEnabling(named_msg &d, deplist &dl, int k, const int* changed)
{
  DCASSERT(k>0);
  int ssz = tdcurr->readSubstateSize(k);
  int next_k = dl.getLevelAbove(k);

  //
  // Are we obligated to visit unexplored states only?
  //
  intset* toVisit = &(confirmed[k]);
  const int* next_changed = 0;
  if (changed) {
    if (k == changed[0]) {
      next_changed = changed+1;
    } else {
      next_changed = changed;
    }
    if (0==next_changed[0]) {
      // visit unexplored only
      toVisit = &(toBeExplored[k]);
    }
  }
  DCASSERT(toVisit);

  //
  // Start exploring.
  // Two cases - bottom level, and not there yet.
  //

  if (next_k > 0) {
    //
    // Not the bottom level; recurse
    //
    tdcurr->set_substate_known(k);
    for (int i = toVisit->getSmallestAfter(-1);
         i>=0;
         i = toVisit->getSmallestAfter(i)) 
    {
      int foo = colls->getSubstate(k, i, tdcurr->writeSubstate(k), ssz);
      if (foo<0) throw subengine::Engine_Failed;

      // 
      // option for short circuiting here...
      //
      
      from_minterm[k] = i;
      exploreEnabling(d, dl, next_k, toBeExplored[k].contains(i) ? 0 : next_changed);
    } // for i
    from_minterm[k] = DONT_CARE;
    tdcurr->set_substate_unknown(k);
    return;
  } 

  // 
  // Bottom level
  // Explore as usual, but instead of recursing, add the minterm.
  //
  tdcurr->set_substate_known(k);
  for (int i = toVisit->getSmallestAfter(-1);
       i>=0;
       i = toVisit->getSmallestAfter(i)) 
  {
    int foo = colls->getSubstate(k, i, tdcurr->writeSubstate(k), ssz);
    if (foo<0) throw subengine::Engine_Failed;

    from_minterm[k] = i;

    bool start_d = d.startReport();
    if (start_d) {
      d.report() << "enabled?\n\tstate ";
      tdcurr->Print(d.report(), 0);
      d.report() << "\n\tminterm [";
      d.report().PutArray(from_minterm+1, num_levels);
      d.report() << "]\n";
    }
    
    bool is_enabled = true;
    for (expr_node* n = dl.termlist; n; n=n->next) {
      n->term->Compute(td);

      if (start_d) {
        d.report() << "\t";
        n->term->Print(d.report(), 0);
        d.report() << " : ";
        if (td.answer->isNormal()) {
          if (td.answer->getBool())
            d.report() << "true";
          else
            d.report() << "false";
        } else if (td.answer->isUnknown())
          d.report() << "?";
        else
          d.report() << "null";
        d.report() << "\n";
        d.stopIO();
      }

      DCASSERT(td.answer->isNormal());
      if (td.answer->getBool()) continue;
      is_enabled = false;
      break;
    } // for n
    if (!is_enabled) continue;

    // enabled; add minterm to buffer
    if (!dl.addMinterm(from_minterm)) throw subengine::Out_Of_Memory;

  } // for i
  from_minterm[k] = DONT_CARE;
  tdcurr->set_substate_unknown(k);
}


void substate_varoption
::exploreNextstate(named_msg &d, deplist &dl, int k, const int* changed)
{
  DCASSERT(k>0);
  int ssz = tdcurr->readSubstateSize(k);
  int next_k = dl.getLevelAbove(k);

  //
  // Are we obligated to visit unexplored states only?
  //
  intset* toVisit = &(confirmed[k]);
  const int* next_changed = 0;
  if (changed) {
    if (k == changed[0]) {
      next_changed = changed+1;
    } else {
      next_changed = changed;
    }
    if (0==next_changed[0]) {
      // visit unexplored only
      toVisit = &(toBeExplored[k]);
    }
  }
  DCASSERT(toVisit);

  //
  // Start exploring.
  // Two cases - bottom level, and not there yet.
  //

  if (next_k > 0) {
    //
    // Not the bottom level; recurse
    //
    tdcurr->set_substate_known(k);
    tdnext->set_substate_known(k);
    for (int i = toVisit->getSmallestAfter(-1);
         i>=0;
         i = toVisit->getSmallestAfter(i)) 
    {
      int foo = colls->getSubstate(k, i, tdcurr->writeSubstate(k), ssz);
      if (foo<0) throw subengine::Engine_Failed;
      colls->getSubstate(k, i, tdnext->writeSubstate(k), ssz);

      // 
      // option for short circuiting here...
      //
      
      from_minterm[k] = i;
      exploreNextstate(d, dl, next_k, toBeExplored[k].contains(i) ? 0 : next_changed);
    } // for i
    from_minterm[k] = DONT_CARE;
    tdcurr->set_substate_unknown(k);
    tdnext->set_substate_unknown(k);
    return;
  } 

  // 
  // Bottom level
  // Explore as usual, but instead of recursing, add the minterm.
  //

  tdcurr->set_substate_known(k);
  tdnext->set_substate_known(k);
  for (int i = toVisit->getSmallestAfter(-1);
       i>=0;
       i = toVisit->getSmallestAfter(i)) 
  {
    int foo = colls->getSubstate(k, i, tdcurr->writeSubstate(k), ssz);
    if (foo<0) throw subengine::Engine_Failed;
    colls->getSubstate(k, i, tdnext->writeSubstate(k), ssz);

    from_minterm[k] = i;

    bool start_d = d.startReport();
    if (start_d) {
      d.report() << "firing?\n\tstate ";
      tdcurr->Print(d.report(), 0);
      d.report() << "\n\tminterm [";
      d.report().PutArray(from_minterm+1, num_levels);
      d.report() << "]\n";
    }
    
    bool is_enabled = true;
    for (expr_node* n = dl.termlist; n; n=n->next) {
      n->term->Compute(td);

      if (start_d) {
        d.report() << "\t";
        n->term->Print(d.report(), 0);
        d.report() << " : ";
        if (td.answer->isNormal()) 
          d.report() << "ok";
        else if (td.answer->isUnknown())
          d.report() << "?";
        else if (td.answer->isOutOfBounds())
          d.report() << "out of bounds";
        else
          d.report() << "null";
        d.report() << "\n";
        d.stopIO();
      }

      if (td.answer->isNormal()) continue;
      is_enabled = false;
      break;
    } // for n
    if (!is_enabled) continue;

    // next state -> minterm
    //
    for (int kk = dl.getLevelAbove(0); 
        kk>=0; kk = dl.getLevelAbove(kk)) {

      to_minterm[kk] = colls->addSubstate(kk, 
          tdnext->readSubstate(kk), tdnext->readSubstateSize(kk)
      );
    
      if (to_minterm[kk]>=0) continue;
      if (-2==to_minterm[kk]) throw subengine::Out_Of_Memory;
      throw subengine::Engine_Failed;
    } // for kk

    //
    // Reporting
    //
    if (d.startReport()) {
      d.report() << "firing\n\tto state ";
      tdnext->Print(d.report(), 0);
      d.report() << "\n\tto minterm [";
      d.report().PutArray(to_minterm+1, num_levels);
      d.report() << "]\n";
      d.stopIO();
    }

    // add minterm to queue
    //
    if (!dl.addMinterm(from_minterm, to_minterm)) 
      throw subengine::Out_Of_Memory;

    // clear out minterm
    //
    for (int kk = dl.getLevelAbove(0); 
        kk>=0; kk = dl.getLevelAbove(kk)) {

      to_minterm[kk] = DONT_CHANGE;
    } // for kk

  } // for i
  from_minterm[k] = DONT_CARE;
  tdcurr->set_substate_unknown(k);
  tdnext->set_substate_unknown(k);
  return;
}



void substate_varoption::updateLevels(named_msg &d, const int* levels)
{
  DCASSERT(tmpLevels);
  if (d.startReport()) {
    d.report() << "updating for levels: ";
    for (int p=0; levels[p]; p++) {
      if (p) {
        d.report() << ", ";
      }
      d.report() << levels[p];
    } 
    d.report() << "\n";
    d.stopIO();
  }
  for (int i=0; i<getParent().getNumEvents(); i++) {
    const model_event* e = getParent().readEvent(i);
    if (d.startReport()) {
      d.report() << "updating event " << e->Name() << "\n";
      d.stopIO();
    }

    //
    // Enablings
    //
    bool new_enablings = false;
    for (deplist* ed = enable_deps[i]; ed; ed = ed->next) {
      ed->intersectLevels(levels, tmpLevels);
      if (0==tmpLevels[0]) continue;
      // need to explore these
      new_enablings = true;
      exploreEnabling(d, *ed, ed->getLevelAbove(0), tmpLevels);
    }

    //
    // Next state
    //
    bool new_firings = false;
    for (deplist* fd = fire_deps[i]; fd; fd = fd->next) {
      fd->intersectLevels(levels, tmpLevels);
      if (0==tmpLevels[0]) continue;
      // need to explore these
      new_firings = true;
      exploreNextstate(d, *fd, fd->getLevelAbove(0), tmpLevels);
    }

    if (d.startReport()) {
      if (new_firings || new_enablings) {
        d.report() << "event " << e->Name() << " has changed\n";
      } else {
        d.report() << "event " << e->Name() << " is unchanged\n";
      }
      d.stopIO();
    }

    if (!(new_firings || new_enablings)) continue;

#ifdef SHOW_MINTERMS
    if (d.startReport()) {
      d.report() << "event " << e->Name() << ":\n";
      d.report() << "\tenabling:\n";
      enable_deps[i]->showMinterms(d.report(), 12);
      d.report() << "\tfiring:\n";
      fire_deps[i]->showMintermPairs(d.report(), 12);
      d.stopIO();
    }
#endif

    // TBD: update the edges from the minterms

  } // for i

  //
  // Done exploring.
  // Mark everything in these levels as explored.
  //
  for (int p=0; levels[p]; p++) {
    int k = levels[p];
    enlarge(toBeExplored[k], colls->getMaxIndex(k));
    toBeExplored[k].removeAll();
    enlarge(confirmed[k], colls->getMaxIndex(k));
  } // for k

#ifdef SHOW_SUBSTATES
  if (d.startReport()) {
    d.report() << "Done updating.  Current substates:\n";
    show_substates(d.report());
    d.stopIO();
  }
#endif
}

#ifdef SHOW_SUBSTATES
void substate_varoption::show_substates(OutputStream &s)
{
  for (int k=num_levels; k>0; k--) {
    s << "    Level " << k << " substates:\n";
    int ssz = tdcurr->readSubstateSize(k);
    tdcurr->set_substate_known(k);
    for (int i=0; i<colls->getMaxIndex(k); i++) {
      s << "\t";
      s.Put(long(i), 3);
      s.Put(confirmed[k].contains(i) ? 'c' : 'u');
      s.Put(toBeExplored[k].contains(i) ? '!' : ' ');
      s << ": ";
      colls->getSubstate(k, i, tdcurr->writeSubstate(k), ssz);
      tdcurr->Print(s, 0);
      s << "\n";
    } // for i
    tdcurr->set_substate_unknown(k);
  } // for k
}
#endif

// **************************************************************************
// *                                                                        *
// *                         pregen_varoption class                         *
// *                                                                        *
// **************************************************************************

class pregen_varoption : public substate_varoption {
public:
  pregen_varoption(meddly_reachset &x, const dsde_hlm &p, 
    const exprman* em, const meddly_procgen &pg);

  virtual void updateEvents(named_msg &d, bool* cl);

  virtual bool hasChangedLevels(const dd_edge &s, bool* cl);
};

// **************************************************************************
// *                                                                        *
// *                        pregen_varoption methods                        *
// *                                                                        *
// **************************************************************************

pregen_varoption
::pregen_varoption(meddly_reachset &x, const dsde_hlm &p, 
  const exprman* em, const meddly_procgen &pg)
: substate_varoption(x, p, em, pg)
{
}

void 
pregen_varoption::updateEvents(named_msg &d, bool* cl)
{
  int* updated = new int[num_levels+1];

  for (int iter=1; ; iter++) { 
    if (d.startReport()) {
      d.report() << "Pregenerating locals, iteration " << iter << "\n";
#ifdef SHOW_SUBSTATES
      d.report() << "Current substates:\n";
      show_substates(d.report());
#endif
      d.stopIO();
    }

    // What levels have been updated
    buildLevelsToExplore(updated);
    if (0==updated[0]) break;   // nothing

    // Adjust enablings, next-state functions for those levels
    updateLevels(d, updated);

    // set unconfirmed to be explored,
    // and confirm everything
    for (int k=num_levels; k>0; k--) {
      toBeExplored[k].addRange(0, colls->getMaxIndex(k)-1);
      toBeExplored[k] -= confirmed[k];
      confirmed[k].addRange(0, colls->getMaxIndex(k)-1);
    }
  }

  // Cleanup
  delete[] updated;
  throw subengine::Engine_Failed;
}

bool pregen_varoption::hasChangedLevels(const dd_edge &s, bool* cl)
{
  return false;  
}


// **************************************************************************
// *                                                                        *
// *                        onthefly_varoption class                        *
// *                                                                        *
// **************************************************************************

class onthefly_varoption : public substate_varoption {
public:
  onthefly_varoption(meddly_reachset &x, const dsde_hlm &p, 
    const exprman* em, const meddly_procgen &pg);

  virtual void updateEvents(named_msg &d, bool* cl);

  virtual bool hasChangedLevels(const dd_edge &s, bool* cl);
};

// **************************************************************************
// *                                                                        *
// *                       onthefly_varoption methods                       *
// *                                                                        *
// **************************************************************************

onthefly_varoption
::onthefly_varoption(meddly_reachset &x, const dsde_hlm &p, 
  const exprman* em, const meddly_procgen &pg) 
 : substate_varoption(x, p, em, pg)
{
}

void onthefly_varoption::updateEvents(named_msg &d, bool* cl)
{
  // throw subengine::Engine_Failed;
}

bool onthefly_varoption::hasChangedLevels(const dd_edge &s, bool* cl)
{
  return false;  // for now...
}


// **************************************************************************
// *                                                                        *
// *                         meddly_procgen methods                         *
// *                                                                        *
// **************************************************************************

int meddly_procgen::proc_storage;
int meddly_procgen::edge_style;
int meddly_procgen::var_type;
int meddly_procgen::nsf_ndp;
int meddly_procgen::rss_ndp;

meddly_procgen::meddly_procgen()
{
  // Anything?
}

meddly_procgen::~meddly_procgen()
{
}

meddly_varoption* 
meddly_procgen::makeBounded(const dsde_hlm &m, meddly_reachset &ms) const
{
  bounded_varoption *mvo = new bounded_varoption(ms, m, em, *this);
  if (!mvo->wasBuiltOK()) {
    delete mvo;
    return 0;
  }
  return mvo;
}


meddly_varoption*
meddly_procgen::makeExpanding(const dsde_hlm &m, meddly_reachset &ms) const
{
  // TBD
  return 0;
}

meddly_varoption*
meddly_procgen::makeOnTheFly(const dsde_hlm &m, meddly_reachset &ms) const
{
  onthefly_varoption *mvo = new onthefly_varoption(ms, m, em, *this);
  if (!mvo->wasBuiltOK()) {
    delete mvo;
    return 0;
  }
  return mvo;
}

meddly_varoption*
meddly_procgen::makePregen(const dsde_hlm &m, meddly_reachset &ms) const
{
  pregen_varoption *mvo = new pregen_varoption(ms, m, em, *this);
  if (!mvo->wasBuiltOK()) {
    delete mvo;
    return 0;
  }
  return mvo;
}

forest::policies 
meddly_procgen::buildNSFPolicies() const
{
  forest::policies p(true);
  switch (nsf_ndp) {
    case NEVER:
      p.setNeverDelete(); 
      break;

    case OPTIMISTIC:
      p.setOptimistic();
      break;

    case PESSIMISTIC:
      p.setPessimistic();
      break;
  } // switch

  return p;
}

forest::policies 
meddly_procgen::buildRSSPolicies() const
{
  forest::policies p(false);
  p.setQuasiReduced();
  switch (rss_ndp) {
    case NEVER:
      p.setNeverDelete(); 
      break;

    case OPTIMISTIC:
      p.setOptimistic();
      break;

    case PESSIMISTIC:
      p.setPessimistic();
      break;
  } // switch

  return p;
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_genmeddly : public initializer {
  public:
    init_genmeddly();
    virtual bool execute();

  private:
    radio_button** makeNDPButtons(int &num_buttons);
};
init_genmeddly the_genmeddly_initializer;

init_genmeddly::init_genmeddly() : initializer("init_genmeddly")
{
  usesResource("em");
  usesResource("engtypes");
  buildsResource("meddlyprocgen");
}

bool init_genmeddly::execute()
{
  if (0==em) return false;

  engtype* rsgen = MakeEngineType(em,
    "MeddlyProcessGeneration",
    "The algorithm to use to generate the underlying process, with Meddly",
    engtype::Model
  );
  engine* meddlygen = MakeRedirectionEngine(
    "MEDDLY",
    "Generate and store the underlying process using MDDs with Meddly; for details see option MeddlyProcessGeneration",
    rsgen
  );
  RegisterEngine(em, "ProcessGeneration", meddlygen);

  // EVMxD vs MTMxD option
  meddly_procgen::proc_storage = meddly_procgen::MTMXD;
  radio_button** styles = new radio_button*[2];
  styles[meddly_procgen::MTMXD] = new radio_button(
    "MTMXD",
    "Multi-terminal MDD for matrices",
    meddly_procgen::MTMXD
  );
  styles[meddly_procgen::EVMXD] = new radio_button(
    "EVMXD",
    "Edge-valued (using *) MDD for matrices",
    meddly_procgen::EVMXD
  );
  em->addOption(
    MakeRadioOption(
      "MeddlyProcessStorage",
      "Type of forest to use for process storage, using Meddly",
      styles, 2, meddly_procgen::proc_storage
    )
  );

  // potential vs. actual edge option
  meddly_procgen::edge_style = meddly_procgen::POTENTIAL;
  styles = new radio_button*[2];
  styles[meddly_procgen::ACTUAL] = new radio_button(
    "ACTUAL",
    "Outgoing edges only for reachable states; unreachable states have no outgoing edges",
    meddly_procgen::ACTUAL
  );
  styles[meddly_procgen::POTENTIAL] = new radio_button(
    "POTENTIAL",
    "Unreachable states may have outgoing edges",
    meddly_procgen::POTENTIAL
  );
  em->addOption(
    MakeRadioOption(
      "MeddlyProcessEdgeStyle",
      "Style for representing the underlying process, using Meddly",
      styles, 2, meddly_procgen::edge_style
    )
  );

  // variable type option
  styles = new radio_button*[4];
  styles[meddly_procgen::BOUNDED] = new radio_button(
    "BOUNDED", 
    "All state variables have declared bounds",
    meddly_procgen::BOUNDED
  );
  styles[meddly_procgen::EXPANDING] = new radio_button(
    "EXPANDING", 
    "State variable bounds are discovered during generation",
    meddly_procgen::EXPANDING
  );
  styles[meddly_procgen::ON_THE_FLY] = new radio_button(
    "ON_THE_FLY", 
    "Local state spaces are discovered during generation",
    meddly_procgen::ON_THE_FLY
  );
  styles[meddly_procgen::PREGEN] = new radio_button(
    "PREGEN", 
    "Local state spaces are generated in before generating reachability set",
    meddly_procgen::PREGEN
  );
  meddly_procgen::var_type = meddly_procgen::BOUNDED;  
  em->addOption(
    MakeRadioOption(
      "MeddlyVariableStyle",
      "Method for determining sizes of state variables, for Meddly-based process generation",
      styles, 4, meddly_procgen::var_type
    )
  );

  // Node deletion policy options
  radio_button** ndps;
  int num_ndps;
  ndps = makeNDPButtons(num_ndps);
  meddly_procgen::nsf_ndp = meddly_procgen::PESSIMISTIC;
  em->addOption(
    MakeRadioOption(
      "MeddlyNSFNodeDeletion",
      "Node deletion policy to use for next-state function forests in Meddly",
      ndps, num_ndps, meddly_procgen::nsf_ndp
    )
  );

  ndps = makeNDPButtons(num_ndps);
  meddly_procgen::rss_ndp = meddly_procgen::PESSIMISTIC;
  em->addOption(
    MakeRadioOption(
      "MeddlyRSSNodeDeletion",
      "Node deletion policy to use for reachable state space forests in Meddly",
      ndps, num_ndps, meddly_procgen::rss_ndp
    )
  );

  meddly_varoption::vars_named = false;
  em->addOption(
    MakeBoolOption(
      "MeddlyVarsAreNamed",
      "Should the variables internal to MEDDLY be named appropriately.  If true, when MDDs and MxDs are displayed (typically for debugging), more useful information is shown for the level names.  Should be set to false for speed if possible.",
      meddly_varoption::vars_named
    )
  );

  return true;
}

radio_button** init_genmeddly::makeNDPButtons(int &num_buttons)
{
  num_buttons = 3;
  radio_button** ndps = new radio_button*[num_buttons];
  ndps[meddly_procgen::NEVER] = new radio_button(
    "NEVER",
    "Never delete nodes; useful for debugging",
    meddly_procgen::NEVER
  );
  ndps[meddly_procgen::OPTIMISTIC] = new radio_button(
    "OPTIMISTIC",
    "Nodes are marked for deletion and are recycled once they do not appear in the compute table; useful when nodes are likely to be re-used",
    meddly_procgen::OPTIMISTIC
  );
  ndps[meddly_procgen::PESSIMISTIC] = new radio_button(
    "PESSIMISTIC",
    "Nodes are recycled as soon as possible; useful when nodes are unlikely to be re-used, or to reduce memory",
    meddly_procgen::PESSIMISTIC
  );
  return ndps;
}


//==========================================================================================
#else // not USING_OLD_MEDDLY_STUFF
//==========================================================================================

//==========================================================================================
#endif
//==========================================================================================
