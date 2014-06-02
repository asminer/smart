
// $Id$

#include "ctl_exp.h"

#include "../ExprLib/engine.h"
#include "../ExprLib/exprman.h"

#include "../Formlsms/stoch_llm.h"
#include "../Formlsms/phase_hlm.h"

#include "../Modules/statesets.h"

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
};

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
  DCASSERT(3==np);
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

  // Determine the initial distribution
  statedist* initial = sm->getInitialDistribution();


  //
  // Parameter 1: p of p U q
  //
  stateset* p = 0;
  if (pass[1].isNormal()) {
    p = smart_cast <stateset*> (pass[1].getPtr());
    DCASSERT(p);
    DCASSERT(p->getParent() == sm);
    DCASSERT(p->isExplicit());
  }

  //
  // Parameter 2: q of p U q
  //
  stateset* q = smart_cast <stateset*> (pass[2].getPtr());
  DCASSERT(q);
  DCASSERT(q->getParent() == sm);
  DCASSERT(q->isExplicit());

  //
  // Build a phase-type model for p U q.
  //
  phase_hlm* tta;
  if (p) {
    // build trap states: not (p or q)
    intset* t = new intset(q->getExplicit());
    (*t) += p->getExplicit();
    t->complement();
    stateset* trap = new stateset(sm, t);
    // build tta model
    tta = makeTTA(discrete, initial,
      q, trap, Share(sm)
    );
    Delete(trap);
  } else {
    // 0 for p means "all true", so no trap states
    tta = makeTTA(discrete, initial,
      q, 0, Share(sm)
    );
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
  inline static void runAvgPh(hldsm* m) {
    if (0==AvgPh) {
      AvgPh = em->findEngineType("AvgPh");
    }
    result dummy;
    if (!AvgPh) throw No_Engine;
    AvgPh->runEngine(m, dummy);
  }
  inline static void 
  generateTU(result* pass, int np, traverse_data &x)
  {
    if (0==TUgen) {
      TUgen = em->findEngineType("TUgenerator");
    }
    if (!TUgen) throw No_Engine;
    TUgen->runEngine(pass, np, x);
  }
};

engtype* PU_expl_eng::AvgPh = 0;
engtype* PU_expl_eng::TUgen = 0;

PU_expl_eng the_PU_expl_eng;

PU_expl_eng::PU_expl_eng() : CSL_expl_eng()
{
}

void PU_expl_eng::RunEngine(result* pass, int np, traverse_data &x)
{
  DCASSERT(3==np);
  DCASSERT(pass[0].isNormal());
  DCASSERT(pass[2].isNormal());

  //
  // Build the distribution by calling the engine
  //
  generateTU(pass, np, x);

  //
  // Grab the distribution
  //
  if (!x.answer->isNormal()) {
    return;
  }
  hldsm* tta = smart_cast <hldsm*>(x.answer->getPtr());
  DCASSERT(tta);

  //
  // call the AvgPh engine
  //
  runAvgPh(tta);

  //
  // Grab the result
  //
  stochastic_lldsm* proc = smart_cast <stochastic_lldsm*> (tta->GetProcess());
  DCASSERT(proc);
  x.answer->setReal( proc->getAcceptProb() );
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



