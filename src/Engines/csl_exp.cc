
// $Id$

#include "ctl_exp.h"

#include "../ExprLib/engine.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"

#include "../Formlsms/stoch_llm.h"
#include "../Formlsms/phase_hlm.h"

#include "../Modules/expl_ssets.h"
#include "../Modules/statevects.h"

#include "intset.h"


// **************************************************************************
// *                                                                        *
// *                           CSL_expl_eng class                           *
// *                                                                        *
// **************************************************************************

/// Abstract base class for explicit CSL engines.
class CSL_expl_eng : public subengine {
public:
  CSL_expl_eng();
  virtual bool AppliesToModelType(hldsm::model_type mt) const;

protected:
  void GenerateProc(hldsm* m) const
  {
    result f;
    f.setBool(false);
    lldsm* proc = m->GetProcess();
    if (proc) {
      subengine* gen = proc->getCompletionEngine();
      if (gen)  gen->RunEngine(m, f);
      if (lldsm::Error == proc->Type()) throw Engine_Failed;
    } else {
      if (0==ProcessGeneration) {
        ProcessGeneration = em->findEngineType("ProcessGeneration");
      }
      if (0==ProcessGeneration) throw No_Engine;
      ProcessGeneration->runEngine(m, f);
      proc = m->GetProcess();
    }
    DCASSERT(proc);
    DCASSERT(proc->Type() == lldsm::DTMC || proc->Type() == lldsm::CTMC);
  }

private:
  static engtype* ProcessGeneration;
};

engtype* CSL_expl_eng::ProcessGeneration = 0;

CSL_expl_eng::CSL_expl_eng() : subengine()
{
}

bool CSL_expl_eng::AppliesToModelType(hldsm::model_type mt) const
{
  return (0==mt); 
}

// **************************************************************************
// *                                                                        *
// *                           TU_generate  class                           *
// *                                                                        *
// **************************************************************************

class TU_generate : public CSL_expl_eng {
public:
  TU_generate();
  virtual void RunEngine(result* pass, int np, traverse_data &x);
};

TU_generate the_TU_generator;

TU_generate::TU_generate() : CSL_expl_eng()
{
}

void TU_generate::RunEngine(result* pass, int np, traverse_data &x)
{
  DCASSERT(4==np);
  DCASSERT(pass[0].isNormal());
  DCASSERT(pass[2].isNormal());

  // 
  // Parameter 0: the model.
  //

  // Make sure it is a stochastic model
  stochastic_lldsm* sm = dynamic_cast <stochastic_lldsm*>(pass[0].getPtr());
  if (0==sm) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "TU distribution requires a stochastic model\n";
      em->stopIO();
    }
    throw Engine_Failed;
  }

  // Make sure it is a MC
  bool discrete;
  switch (sm->Type()) {
    case lldsm::GSP:
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "TU distribution requires an underlying Markov chain\n";
        em->stopIO();
      }
      throw Engine_Failed;

    case lldsm::DTMC:
      discrete = true;
      break;

    case lldsm::CTMC:
      discrete = false;
      break;

    default:
      // Anything else should be impossible, right?
      DCASSERT(0);
      throw Engine_Failed;
  };

  //
  // Parameter 1: p of p U q
  //
#ifdef NEW_STATESETS
  expl_stateset* p = 0;
#else
  stateset* p = 0;
#endif
  if (pass[1].isNormal()) {
#ifdef NEW_STATESETS
    p = smart_cast <expl_stateset*> (pass[1].getPtr());
#else
    p = smart_cast <stateset*> (pass[1].getPtr());
#endif
    DCASSERT(p);
    DCASSERT(p->getParent() == sm);
  }

  //
  // Parameter 2: q of p U q
  //
#ifdef NEW_STATESETS
  expl_stateset* q = smart_cast <expl_stateset*> (pass[2].getPtr());
#else
  stateset* q = smart_cast <stateset*> (pass[2].getPtr());
#endif
  DCASSERT(q);
  DCASSERT(q->getParent() == sm);

  //
  // Parameter 3: initial distribution, or null for uniform
  //
  statedist* initial = 0;
  if (pass[3].isNormal()) {
    initial = Share(smart_cast <statedist*>(pass[3].getPtr()));
    DCASSERT(initial);
    DCASSERT(initial->getParent() == sm);
  } 

  //
  // Build a phase-type model for p U q.
  //
  phase_hlm* tta;
  if (p) {
    // build trap states: not (p or q)
    intset* t = new intset(q->getExplicit());
    (*t) += p->getExplicit();
    t->complement();
#ifdef NEW_STATESETS
    stateset* trap = new expl_stateset(sm, t);
#else
    stateset* trap = new stateset(sm, t);
#endif
    // build tta model
    tta = makeTTA(discrete, initial, q, trap, Share(sm));
    Delete(trap);
  } else {
    // 0 for p means "all true", so no trap states
    tta = makeTTA(discrete, initial, q, 0, Share(sm));
  }
  if (0==tta) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->causedBy(x.parent);
      em->internal() << "Couldn't build TTA phase model\n";
      em->stopIO();
    }
    throw Engine_Failed;
  }

  x.answer->setPtr(tta);
}



// **************************************************************************
// *                                                                        *
// *                           PU_expl_eng  class                           *
// *                                                                        *
// **************************************************************************

class PU_expl_eng : public CSL_expl_eng {
  static engtype* AvgPh;
  static engtype* TUgen;
public:
  PU_expl_eng();
  virtual void RunEngine(result* pass, int np, traverse_data &x);
protected:
  inline static void 
  generateTU(result* pass, int np, traverse_data &x)
  {
    if (0==TUgen) {
      TUgen = em->findEngineType("TUgenerator");
    }
    if (!TUgen) throw No_Engine;
    TUgen->runEngine(pass, np, x);
  }

protected:
  class reindex : public lldsm::state_visitor {
      const double* oldvec;
      long oldvecsize;
      double* newvec;
      long newvecsize;
    public:
      reindex(const hldsm* p, const double* ov, long os, double* nv, long ns) 
      : lldsm::state_visitor(p)
      {
        oldvec = ov;
        oldvecsize = os;
        newvec = nv;
        newvecsize = ns;
      }
      virtual ~reindex() { }

      virtual bool visit() {
        long ni = state()->get(0); 
        if (ni >= newvecsize) return false; // must be trap or accept
        //fprintf(stderr, "Converting from %ld to %ld\n", index(), ni);
        CHECK_RANGE(0, index(), oldvecsize);
        CHECK_RANGE(0, ni, newvecsize);
        newvec[ni] = oldvec[index()];
        return false;
      }
  };
};

engtype* PU_expl_eng::AvgPh = 0;
engtype* PU_expl_eng::TUgen = 0;

PU_expl_eng the_PU_expl_eng;

PU_expl_eng::PU_expl_eng() : CSL_expl_eng()
{
}

void PU_expl_eng::RunEngine(result* pass, int np, traverse_data &x)
{
  //
  // Grab the time parameter
  //
  bool unbounded = false;
  double time = 0;
  if (pass[3].isNormal()) {
    time = pass[3].getReal();
  } else if (pass[3].isInfinity()) {
    if (pass[3].signInfinity() > 0) {
      // + infinity
      unbounded = true;
    } else {
      // - infinity
      time = 0;
    }
  } else {
    // null or some other bizarre thing
    throw subengine::Bad_Value;
  }

  pass[3].setNull();

  //
  // Build the distribution by calling the TU engine
  //
  generateTU(pass, np, x);

  //
  // Grab the phase model
  //
  if (!x.answer->isNormal()) {
    return;
  }
  phase_hlm* tta = smart_cast <phase_hlm*>(x.answer->getPtr());
  DCASSERT(tta);

  //
  // Generate the process
  //
  GenerateProc(tta);
  stochastic_lldsm* ttamc = smart_cast <stochastic_lldsm*> (tta->GetProcess());
  DCASSERT(ttamc);


  //
  // Allocate vector for absorbing chain answer
  //
  long ttans = ttamc->getNumStates();
  double* ttax = new double[ttans];

  //
  // Do computation
  //
  bool ok;
  if (unbounded)  ok = ttamc->reachesAccept(ttax);
  else            ok = ttamc->reachesAcceptBy(time, ttax);
  if (!ok) {
    delete[] ttax;
    x.answer->setNull();
    return;
  }

  //
  // Convert from phase model indexes back to the original model
  //
  const stochastic_lldsm* sm 
  = dynamic_cast <const stochastic_lldsm*>(pass[0].getPtr());

  long mns = sm->getNumStates();
  double* mx = new double[mns];
  for (long i=0; i<mns; i++) mx[i] = 0;

  reindex foo(ttamc->GetParent(), ttax, ttans, mx, mns);
  ttamc->visitStates(foo);

  //
  // Done with ttax
  //
  delete[] ttax;
  
  //
  // Set the accept equivalent states to 1 here
  //
#ifdef NEW_STATESETS
  expl_stateset* q = smart_cast <expl_stateset*> (pass[2].getPtr());
#else
  stateset* q = smart_cast <stateset*> (pass[2].getPtr());
#endif
  DCASSERT(q);
  const intset& qis = q->getExplicit();
  long i = -1;
  while ( (i=qis.getSmallestAfter(i)) >= 0) {
    mx[i] = 1;
  }

  //
  // vector is set, convert it to a stateprobs
  //
  stateprobs* ans = new stateprobs(sm, mx, mns);
  delete[] mx;

  x.answer->setPtr(ans);
}


// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeExplicitCSLEngines(exprman* em)
{
  if (0==em) return;

  RegisterEngine(
      em,
      "TUgenerator",
      "process", 
      "Use the underlying process to generate a TU distribution",
      &the_TU_generator
  );

  RegisterEngine(
      em,
      "ExplicitPU",
      "phase_tta", 
      "Use a phase-type tta operation to compute PU",
      &the_PU_expl_eng 
  );
}



