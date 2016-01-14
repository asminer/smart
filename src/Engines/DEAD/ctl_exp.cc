
// $Id$

#include "ctl_exp.h"

#include "../ExprLib/engine.h"
#include "../ExprLib/exprman.h"

#include "../Formlsms/graph_llm.h"

#include "../Modules/statesets.h"

#include "intset.h"

// #define DEBUG_EF
// #define DEBUG_EU
// #define DEBUG_EG
// #define PROGRESS_EG
#define DEBUG_AEF

#ifndef NEW_STATESETS

void Show(const char* who, const intset &x)
{
  fprintf(stderr, "%s{", who);
  bool printed = false;
  for (long i=0; i<x.getSize(); i++) {
    if (!x.contains(i)) continue;
    if (printed) fprintf(stderr, ", ");
    printed = true;
    fprintf(stderr, "%ld", i);
  }
  fprintf(stderr, "}\n");
}

inline void Show(const char* who, stateset &sx)
{
  Show(who, sx.getExplicit());
}

inline intset* GrabInitial(const graph_lldsm* mdl)
{
  intset* iis = 0;
  result init;
  mdl->getInitialStates(init);
  stateset* i = smart_cast <stateset*> (init.getPtr());
  if (i) {
    DCASSERT(i->isExplicit());
    DCASSERT(i->getParent() == mdl);
    iis = new intset(i->getExplicit());
  } 
  return iis;
}

// **************************************************************************
// *                                                                        *
// *                      Basic engines used by others                      *
// *                                                                        *
// **************************************************************************

inline long explicit_ES(const graph_lldsm* mdl, const stateset &p, const stateset &q, stateset &r, stateset &tmp)
{
  r.changeExplicit().assignFrom(q.getExplicit());
  // E p S q
  long lastcard = 0;
  long iters;
  for (iters = 1; ; iters++) {
      tmp.changeExplicit().removeAll();
      mdl->forward(r.getExplicit(), tmp.changeExplicit());
      // intersect with p
      tmp.changeExplicit() *= p.getExplicit();
      // add to r
      r.changeExplicit() += tmp.changeExplicit();
      long newcard = r.getExplicit().cardinality();
      if (newcard == lastcard) break;
      lastcard = newcard;
#ifdef DEBUG_EU
      Show("r: ", r);
#endif
  }
  // add back q states
  r.changeExplicit() += q.getExplicit();
  return iters;
}

inline long explicit_EU(const graph_lldsm* mdl, const stateset &p, const stateset &q, stateset &r, stateset &tmp)
{
  r.changeExplicit().assignFrom(q.getExplicit());
  // E p U q
  long lastcard = 0;
  long iters;
  for (iters = 1; ; iters++) {
      tmp.changeExplicit().removeAll();
      mdl->backward(r.getExplicit(), tmp.changeExplicit());
      // intersect with p
      tmp.changeExplicit() *= p.getExplicit();
      // add to r
      r.changeExplicit() += tmp.changeExplicit();
      long newcard = r.getExplicit().cardinality();
      if (newcard == lastcard) break;
      lastcard = newcard;
#ifdef DEBUG_EU
      Show("r: ", r);
#endif
  }
  // add back q states
  r.changeExplicit() += q.getExplicit();
  return iters;
}

// Forward reachable
inline long forward_closure(const graph_lldsm* mdl, stateset &r)
{
  for (long iters=1; ; iters++) {
#ifdef DEBUG_EF
      Show("r: ", r);
#endif
      bool change = mdl->forward(r.getExplicit(), r.changeExplicit());
      if (!change) return iters;
  }
}

// Backward reachable
inline long backward_closure(const graph_lldsm* mdl, stateset &r)
{
  for (long iters=1; ; iters++) {
#ifdef DEBUG_EF
      Show("r: ", r);
#endif
      bool change = mdl->backward(r.getExplicit(), r.changeExplicit());
      if (!change) return iters;
  }
}


inline long unfair_EH(const graph_lldsm* mdl, const stateset &p, stateset &r, stateset &tmp)
{
#ifdef PROGRESS_EG
  fprintf(stderr, "Starting unfair_EH engine\n");
#endif

  // build set of source states satisfying p
  intset* dps = GrabInitial(mdl);
  if (dps) *dps *= p.getExplicit();
  if (dps) {
#ifdef DEBUG_EG
    Show("dps: ", *dps);
#endif
    // if dps is empty, trash it.
    if (0==dps->cardinality()) {
      delete dps;
      dps = 0;
    }
  }

  r.changeExplicit().assignFrom(p.getExplicit());
  long card = 0;
  long iters = 0;
  for (;;) {
#ifdef PROGRESS_EG
    fprintf(stderr, "EH checking termination condition\n");
#endif
    long isrcard = r.getExplicit().cardinality();
    if (isrcard == card) break;
    card = isrcard;

    iters++;
#ifdef PROGRESS_EG
    fprintf(stderr, "EH starting iteration %ld\n", iters);
#endif
#ifdef DEBUG_EG
    Show("r: ", r);
#endif

    tmp.changeExplicit().removeAll();
    mdl->forward(r.getExplicit(), tmp.changeExplicit());
    if (dps) tmp.changeExplicit() += (*dps);
#ifdef DEBUG_EG
    Show("tmp: ", tmp);
#endif
    r.changeExplicit() = tmp.getExplicit() * p.getExplicit();
  }
#ifdef PROGRESS_EG
  fprintf(stderr, "EH converged with cardinality %ld\n", card);
#endif
  delete dps;
  return iters;
}


inline long unfair_EG(const graph_lldsm* mdl, const stateset &p, stateset &r, stateset &tmp)
{
#ifdef PROGRESS_EG
  fprintf(stderr, "Starting unfair_EG engine\n");
#endif

  // build set of deadlocked states satisfying p
  stateset* dps = new stateset(mdl, new intset(p.getExplicit()));
  mdl->findDeadlockedStates(*dps);
#ifdef PROGRESS_EG
  fprintf(stderr, "EG: determined deadlocked states\n");
#endif

  if (dps) {
#ifdef DEBUG_EG
    Show("dps: ", *dps);
#endif
    // if dps is empty, trash it.
    long dcard = dps->getExplicit().cardinality();
#ifdef PROGRESS_EG
    fprintf(stderr, "EG: there are %ld deadlocked states\n", dcard);
#endif
    if (0==dcard) {
      delete dps;
      dps = 0;
    }
  }

  r.changeExplicit().assignFrom(p.getExplicit());
  long card = 0;
  long iters = 0;
  for (;;) {
#ifdef PROGRESS_EG
    fprintf(stderr, "EG checking termination condition\n");
#endif
    long isrcard = r.getExplicit().cardinality();
    if (isrcard == card) break;
    card = isrcard;

    iters++;
#ifdef PROGRESS_EG
    fprintf(stderr, "EG starting iteration %ld\n", iters);
#endif
#ifdef DEBUG_EG
    Show("r: ", r);
#endif

    tmp.changeExplicit().removeAll();
    mdl->backward(r.getExplicit(), tmp.changeExplicit());
    if (dps) tmp.changeExplicit() += dps->getExplicit();
#ifdef DEBUG_EG
    Show("tmp: ", tmp);
#endif
    r.changeExplicit() = tmp.getExplicit() * p.getExplicit();
  }
#ifdef PROGRESS_EG
  fprintf(stderr, "EG converged with cardinality %ld\n", card);
#endif
  delete dps;
  return iters;
}

// **************************************************************************
// *                                                                        *
// *                           CTL_expl_eng class                           *
// *                                                                        *
// **************************************************************************

//
// TBD - 
//
// Define an abstract base class for all CTL engines,
// and move the "report" option there.
// Need to make this class visible to the symbolic CTL engines,
// so probably put it in the header file.
//

/// Abstract base class for explicit CTL engines.
class CTL_expl_eng : public subengine {
public:
  CTL_expl_eng();
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
private:
  static named_msg ctl_report;
  friend void InitializeExplicitCTLEngines(exprman* em);
protected:
  inline void reportIters(const char* who, long iters) {
    if (!ctl_report.startReport()) return;
    ctl_report.report() << who << " required " << iters << " iterations\n";
    em->stopIO();
  }
};

named_msg CTL_expl_eng::ctl_report;

CTL_expl_eng::CTL_expl_eng() : subengine()
{
}

bool CTL_expl_eng::AppliesToModelType(hldsm::model_type mt) const
{
  return (0==mt); 
}

// **************************************************************************
// *                                                                        *
// *                           EX_expl_eng  class                           *
// *                                                                        *
// **************************************************************************

class EX_expl_eng : public CTL_expl_eng {
public:
  EX_expl_eng();
  virtual void RunEngine(result* pass, int np, traverse_data &x);
};

EX_expl_eng the_EX_expl_eng;

EX_expl_eng::EX_expl_eng() : CTL_expl_eng()
{
}

void EX_expl_eng::RunEngine(result* pass, int np, traverse_data &x)
{
  DCASSERT(2==np);
  DCASSERT(pass[0].isNormal());
  DCASSERT(pass[1].isNormal());
  stateset* p = smart_cast <stateset*> (pass[1].getPtr());
  DCASSERT(p);
  const graph_lldsm* mdl = 
    smart_cast <const graph_lldsm*>(p->getParent());
  DCASSERT(mdl);

  intset* isr = new intset(p->getExplicit().getSize());
  isr->removeAll();
  stateset* r = new stateset(mdl, isr);
  if (pass[0].getBool()) mdl->forward(p->getExplicit(), r->changeExplicit());
  else                   mdl->backward(p->getExplicit(), r->changeExplicit());  
  x.answer->setPtr(r);
}

// **************************************************************************
// *                                                                        *
// *                           EU_expl_eng  class                           *
// *                                                                        *
// **************************************************************************

class EU_expl_eng : public CTL_expl_eng {
public:
  EU_expl_eng();
  virtual void RunEngine(result* pass, int np, traverse_data &x);
};

EU_expl_eng the_EU_expl_eng;

EU_expl_eng::EU_expl_eng() : CTL_expl_eng()
{
}

void EU_expl_eng::RunEngine(result* pass, int np, traverse_data &x)
{
  DCASSERT(3==np);
  DCASSERT(pass[0].isNormal());
  DCASSERT(pass[2].isNormal());
  
  stateset* q = smart_cast <stateset*> (pass[2].getPtr());
  DCASSERT(q);
  DCASSERT(q->isExplicit());
  const graph_lldsm* mdl = 
    smart_cast <const graph_lldsm*>(q->getParent());
  stateset* p = 0;

  if (pass[1].isNormal()) {
    p = smart_cast <stateset*> (pass[1].getPtr());
    DCASSERT(p);
    DCASSERT(p->getParent() == mdl);
  }

  stateset* r = new stateset(mdl, new intset(q->getExplicit().getSize()));
  if (p) {
    stateset tmp(mdl, new intset(q->getExplicit().getSize()));
    if (pass[0].getBool()) {
      reportIters("ES", explicit_ES(mdl, *p, *q, *r, tmp));
    } else {
      reportIters("EU", explicit_EU(mdl, *p, *q, *r, tmp));
    }
  } else {
    r->changeExplicit() = q->getExplicit();
    if (pass[0].getBool()) {
      reportIters("EH", forward_closure(mdl, *r));
    } else {
      reportIters("EF", backward_closure(mdl, *r));
    }
  }
  
  x.answer->setPtr(r);
}

// **************************************************************************
// *                                                                        *
// *                        unfairEG_expl_eng  class                        *
// *                                                                        *
// **************************************************************************

class unfairEG_expl_eng : public CTL_expl_eng {
public:
  unfairEG_expl_eng();
  virtual void RunEngine(result* pass, int np, traverse_data &x);
};

unfairEG_expl_eng the_unfairEG_expl_eng;

unfairEG_expl_eng::unfairEG_expl_eng() : CTL_expl_eng()
{
}

void unfairEG_expl_eng::RunEngine(result* pass, int np, traverse_data &x)
{
  DCASSERT(2==np);
  DCASSERT(pass[0].isNormal());
  DCASSERT(pass[1].isNormal());
  stateset* p = smart_cast <stateset*> (pass[1].getPtr());
  DCASSERT(p);
  DCASSERT(p->isExplicit());
  const graph_lldsm* mdl = 
    smart_cast <const graph_lldsm*>(p->getParent());
  DCASSERT(mdl);

#ifdef DEBUG_EG
  Show("p: ", *p);
#endif

  stateset* r = new stateset(mdl, new intset(p->getExplicit().getSize()));
  stateset tmp(mdl, new intset(p->getExplicit().getSize()));
  if (pass[0].getBool()) {
    reportIters("Unfair EH", unfair_EH(mdl, *p, *r, tmp));
  } else {
    reportIters("Unfair EG", unfair_EG(mdl, *p, *r, tmp));
  }
  x.answer->setPtr(r);
}

// **************************************************************************
// *                                                                        *
// *                         fairEG_expl_eng  class                         *
// *                                                                        *
// **************************************************************************

class fairEG_expl_eng : public CTL_expl_eng {
public:
  fairEG_expl_eng();
  virtual void RunEngine(result* pass, int np, traverse_data &x);
};

fairEG_expl_eng the_fairEG_expl_eng;

fairEG_expl_eng::fairEG_expl_eng() : CTL_expl_eng()
{
}

void fairEG_expl_eng::RunEngine(result* pass, int np, traverse_data &x)
{
  DCASSERT(2==np);
  DCASSERT(pass[0].isNormal());
  DCASSERT(pass[1].isNormal());
  stateset* p = smart_cast <stateset*> (pass[1].getPtr());
  DCASSERT(p);
  DCASSERT(p->isExplicit());
  const graph_lldsm* mdl = 
    smart_cast <const graph_lldsm*>(p->getParent());
  DCASSERT(mdl);

#ifdef DEBUG_EG
  Show("isp: ", *p);
#endif
  stateset* r = new stateset(mdl, new intset(p->getExplicit()));
  mdl->getTSCCsSatisfying(*r);
  if (!pass[0].getBool()) {
    // add states satisfying E (isp U dps)
    stateset tmp(mdl, new intset(p->getExplicit().getSize()));
    long card = 0;
    for (;;) {
      long isrcard = r->getExplicit().cardinality();  
      if (isrcard == card) break;
      card = isrcard;
#ifdef DEBUG_EG
      Show("isr: ", *r);
#endif
      tmp.changeExplicit().removeAll();
      mdl->backward(r->getExplicit(), tmp.changeExplicit());
      tmp.changeExplicit() *= p->getExplicit();
      r->changeExplicit() += tmp.getExplicit();
    } // for ;;
  } else {
    // add "source" states satisfying p, and forward reachable along p.
    intset* init = GrabInitial(mdl);
    if (init) {
      stateset tmp(mdl, init);
      tmp.changeExplicit() *= p->getExplicit();
      long card = r->getExplicit().cardinality();
      for (;;) {
#ifdef DEBUG_EG
        Show("tmp: ", tmp);
#endif
        r->changeExplicit() += tmp.getExplicit();
        long isrcard = r->getExplicit().cardinality();
        if (isrcard == card) break;
        card = isrcard;
#ifdef DEBUG_EG
        Show("isr: ", *r);
#endif
        tmp.changeExplicit().removeAll();
        mdl->forward(r->getExplicit(), tmp.changeExplicit());
        tmp.changeExplicit() *= p->getExplicit();
      }
    } // if init
  }
  x.answer->setPtr(r);
}

// **************************************************************************
// *                                                                        *
// *                        unfairAEF_expl_eng class                        *
// *                                                                        *
// **************************************************************************

class unfairAEF_expl_eng : public CTL_expl_eng {
public:
  unfairAEF_expl_eng();
  virtual void RunEngine(result* pass, int np, traverse_data &x);
};

unfairAEF_expl_eng the_unfairAEF_expl_eng;

unfairAEF_expl_eng::unfairAEF_expl_eng() : CTL_expl_eng()
{
}

void 
unfairAEF_expl_eng::RunEngine(result* pass, int np, traverse_data &x)
{
  DCASSERT(3==np);
  DCASSERT(pass[0].isNormal());
  DCASSERT(pass[1].isNormal());
  DCASSERT(pass[2].isNormal());

  graph_lldsm* mdl = smart_cast <graph_lldsm*> (pass[0].getPtr());
  DCASSERT(mdl);
  
  stateset* p = smart_cast <stateset*> (pass[1].getPtr());
  DCASSERT(p);
  DCASSERT(p->isExplicit());
  DCASSERT(p->getParent() == mdl);

  stateset* q = smart_cast <stateset*> (pass[2].getPtr());
  DCASSERT(q);
  DCASSERT(q->isExplicit());
  DCASSERT(q->getParent() == mdl);

  stateset tmp(mdl, new intset(q->getExplicit().getSize()));
  stateset *r = new stateset(mdl, new intset(q->getExplicit()));
  stateset *ans = new stateset(mdl, new intset(q->getExplicit().getSize()));
  // isr = E isp U isq
  explicit_EU(mdl, *p, *q, *r, tmp);

#ifdef DEBUG_AEF
  Show("r: ", *r);
#endif

  for (;;) {
    // ans = AF isr
    r->changeExplicit().complement();
    unfair_EG(mdl, *r, *ans, tmp);
    if (r->getExplicit() == ans->getExplicit()) {
      ans->changeExplicit().complement();
      break;
    }
    ans->changeExplicit().complement();
    SWAP(ans, r);
#ifdef DEBUG_AEF
    Show("r: ", *r);
#endif

    // ans = E isp U isr
    explicit_EU(mdl, *p, *r, *ans, tmp);
    if (r->getExplicit() == ans->getExplicit()) { 
      break;
    }
    SWAP(ans, r);
#ifdef DEBUG_AEF
    Show("r: ", *r);
#endif
  }
  Delete(r);

  x.answer->setPtr(ans);
}

// **************************************************************************
// *                                                                        *
// *                        specializedAEF_eng class                        *
// *                                                                        *
// **************************************************************************

class specializedAEF_eng : public CTL_expl_eng {
public:
  specializedAEF_eng();
  virtual void RunEngine(result* pass, int np, traverse_data &x);
};

specializedAEF_eng the_specializedAEF_eng;

specializedAEF_eng::specializedAEF_eng() : CTL_expl_eng()
{
}

void 
specializedAEF_eng::RunEngine(result* pass, int np, traverse_data &x)
{
  DCASSERT(3==np);
  DCASSERT(pass[0].isNormal());
  DCASSERT(pass[1].isNormal());
  DCASSERT(pass[2].isNormal());
  DCASSERT(x.answer);
  x.answer->setNull();

  graph_lldsm* mdl = smart_cast <graph_lldsm*> (pass[0].getPtr());
  DCASSERT(mdl);
  if (!mdl->requireByCols(0)) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Couldn't transpose graph\n";
      em->stopIO();
    }
    throw Engine_Failed;
  }

  stateset* p = smart_cast <stateset*> (pass[1].getPtr());
  DCASSERT(p);
  DCASSERT(p->isExplicit());
  DCASSERT(p->getParent() == mdl);

  stateset* q = smart_cast <stateset*> (pass[2].getPtr());
  DCASSERT(q);
  DCASSERT(q->isExplicit());
  DCASSERT(q->getParent() == mdl);

  // initialize queue
  long* queue = new long[p->getExplicit().getSize()];
  long qfront = 0;
  long qtail = 0;

  // initialize required notifications: arc counts.
  long* required = new long[p->getExplicit().getSize()];
  for (long s=p->getExplicit().getSize()-1; s>=0; s--) required[s] = 0;
  if (!mdl->getOutgoingCounts(required)) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Couldn't get edge counts\n";
      em->stopIO();
    }
    delete[] required;
    throw Engine_Failed;
  }

  // adjust for goal, controllable, and deadlocked states.
  for (long s=p->getExplicit().getSize()-1; s>=0; s--) {
    if (0==required[s] || p->getExplicit().contains(s)) {
      required[s] = 1;
    }
    if (q->getExplicit().contains(s)) {
      required[s] = 0;
      // add to queue
      CHECK_RANGE(0, qtail, p->getExplicit().getSize());
      queue[qtail] = s;
      qtail++;
    }
  } // for s

  
  // Explore loop
  ObjectList <int> edges;
  while (qfront < qtail) {

    // remove some state from queue
    CHECK_RANGE(0, qfront, p->getExplicit().getSize());
    long s = queue[qfront];
    qfront++;

    // notify all edges to state s
    edges.Clear();
    mdl->getIncomingEdges(s, &edges);
    for (long z=0; z<edges.Length(); z++) {
      long r = edges.Item(z);
      CHECK_RANGE(0, r, p->getExplicit().getSize());
      if (0==required[r]) continue;
      required[r]--;
      if (required[r]) continue;
      // add r to queue
      CHECK_RANGE(0, qtail, p->getExplicit().getSize());
      queue[qtail] = r;
      qtail++;
    } // for z

  } // while queue not empty

  
  // everything that was ever in the queue, is a solution
  stateset* ans = new stateset(mdl, new intset(p->getExplicit().getSize()));
  ans->changeExplicit().removeAll();
  DCASSERT(qtail <= p->getExplicit().getSize());
  for (long z=0; z<qtail; z++) {
    ans->changeExplicit().addElement(queue[z]);
  }

  // cleanup
  delete[] required;
  delete[] queue;
  x.answer->setPtr(ans);
}

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeExplicitCTLEngines(exprman* em)
{
  if (0==em) return;

  option* report = em->findOption("Report");
  CTL_expl_eng::ctl_report.Initialize(report,
    "CTL_engines",
    "When set, CTL engine performance is reported.",
    false
  );

  RegisterEngine(
      em,
      "ExplicitEX",
      "multiply", 
      "Use traditional matrix-vector multiplication",
      &the_EX_expl_eng
  );
  RegisterEngine(
      em,
      "ExplicitEU",
      "multiply", 
      "Use iterations based on traditional matrix-vector multiplication",
      &the_EU_expl_eng 
  );
  RegisterEngine(
      em,
      "ExplicitUnfairEG",
      "iterative", 
      "Use iterations based on traditional matrix-vector multiplication",
      &the_unfairEG_expl_eng 
  );
  RegisterEngine(
      em,
      "ExplicitFairEG",
      "TSCCs", 
      "Compute TSCCs and then iterate as appropriate",
      &the_fairEG_expl_eng 
  );
  RegisterEngine(
      em,
      "ExplicitUnfairAEF",
      "specialized", 
      "Andy's new algorithm",
      &the_specializedAEF_eng 
  );
  RegisterEngine(
      em,
      "ExplicitUnfairAEF",
      "iterative", 
      "Iterate based on EU and AF operations",
      &the_unfairAEF_expl_eng 
  );
}

#else

void InitializeExplicitCTLEngines(exprman* em)
{
}

#endif

