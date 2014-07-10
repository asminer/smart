
// $Id$

#include "ctl_symb.h"

#include "../ExprLib/engine.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/dd_front.h"
#include "../ExprLib/mod_vars.h"

#include "../Formlsms/check_llm.h"

#include "../Modules/statesets.h"

// #define DEBUG_EG

inline const shared_object* GrabSOInitial(const checkable_lldsm* mdl)
{
  result init;
  mdl->getInitialStates(init);
  stateset* i = smart_cast <stateset*> (init.getPtr());
  if (i) {
    DCASSERT(i->isSymbolic());
    DCASSERT(i->getParent() == mdl);
    return i->getStateDD();
  } 
  return 0;
}

void Show(DisplayStream &cout, const char* who, stateset &x)
{
  cout << who;
  cout << "{";

  shared_state* st = new shared_state(x.getParent()->GetParent());

  bool comma = false;
  const int* mt = x.getStateForest()->firstMinterm(x.changeStateDD());
  while (mt) {
    if (comma)  cout << ", ";
    else        comma = true;
    x.getStateForest()->minterm2state(mt, st);
    x.getParent()->GetParent()->showState(cout, st);
    mt = x.getStateForest()->nextMinterm(x.changeStateDD());
  }

  Delete(st);

  cout << "}\n";
  cout.flush();
}

// **************************************************************************
// *                                                                        *
// *                      Basic engines used by others                      *
// *                                                                        *
// **************************************************************************

inline void 
symbolic_ES(const checkable_lldsm* mdl, const stateset &p, const stateset &q, stateset &ans)
{
  // E p S q iterations

  // Auxiliary vars
  shared_object* prev = ans.getStateForest()->makeEdge(0);
  shared_object* f = ans.getStateForest()->makeEdge(0);

  // prev := emptyset
  ans.getStateForest()->buildSymbolicConst(false, prev);

  // ans := q
  ans.getStateForest()->copyEdge(q.getStateDD(), ans.changeStateDD());

  while (!prev->Equals(ans.getStateDD())) {
    // f := postimage(ans, R)
    ans.getStateForest()->postImage(
              ans.getStateDD(), ans.getRelationDD(), f
    );

    // f := f ^ p
    ans.getStateForest()->buildAssoc(
              f, false, exprman::aop_and, p.getStateDD(), f
    );

    // prev := answer
    ans.getStateForest()->copyEdge(ans.getStateDD(), prev);

    // answer := answer U f
    ans.getStateForest()->buildAssoc(
            ans.getStateDD(), false, exprman::aop_or, f, ans.changeStateDD()
    );
  } // while

  // Cleanup
  Delete(f);
  Delete(prev);
}

inline void 
symbolic_EU(const checkable_lldsm* mdl, const stateset &p, const stateset &q, stateset &ans)
{
  // E p U q iterations

  // Auxiliary vars
  shared_object* prev = ans.getStateForest()->makeEdge(0);
  shared_object* f = ans.getStateForest()->makeEdge(0);

  // prev := emptyset
  ans.getStateForest()->buildSymbolicConst(false, prev);

  // ans := q
  ans.getStateForest()->copyEdge(q.getStateDD(), ans.changeStateDD());

  while (!prev->Equals(ans.getStateDD())) {
    // f := preimage(ans, R)
    ans.getStateForest()->preImage(
              ans.getStateDD(), ans.getRelationDD(), f
    );

    // f := f ^ p
    ans.getStateForest()->buildAssoc(
              f, false, exprman::aop_and, p.getStateDD(), f
    );

    // prev := answer
    ans.getStateForest()->copyEdge(ans.getStateDD(), prev);

    // answer := answer U f
    ans.getStateForest()->buildAssoc(
            ans.getStateDD(), false, exprman::aop_or, f, ans.changeStateDD()
    );
  } // while

  // Cleanup
  Delete(f);
  Delete(prev);
}


inline void 
unfair_EH(const checkable_lldsm* mdl, const stateset &p, stateset &r)
{
  // Auxiliary
  shared_object* oldr = r.getStateForest()->makeEdge(0);
  shared_object* psrc = r.getStateForest()->makeEdge(0);
  shared_object* f = r.getStateForest()->makeEdge(0);

  // oldr := emptyset
  r.getStateForest()->buildSymbolicConst(false, oldr);

  // build set of source states satisfying p
  r.getStateForest()->buildAssoc(
    GrabSOInitial(mdl), false, exprman::aop_and, p.getStateDD(), psrc
  );

  // r := p
  r.getStateForest()->copyEdge(p.getStateDD(), r.changeStateDD());
  
  while (!oldr->Equals(r.getStateDD())) {
    // f := postimage(r, R)
    r.getStateForest()->postImage(
            r.getStateDD(), r.getRelationDD(), f
    );

    // f := f + psrc
    r.getStateForest()->buildAssoc(
            f, false, exprman::aop_or, psrc, f
    );

    // oldr := r
    r.getStateForest()->copyEdge(r.getStateDD(), oldr);

    // r := f * p
    r.getStateForest()->buildAssoc(
            f, false, exprman::aop_and, p.getStateDD(), r.changeStateDD()
    );
  } // while

  // Cleanup
  Delete(psrc);
  Delete(oldr);
  Delete(f);
}

inline void 
unfair_EG(const checkable_lldsm* mdl, const stateset &p, stateset &r)
{
  // Auxiliary
  shared_object* oldr = r.getStateForest()->makeEdge(0);
  shared_object* f = r.getStateForest()->makeEdge(0);
  stateset pdead( 
    mdl, Share(r.getStateForest()), 
    r.getStateForest()->makeEdge(p.getStateDD()),
    Share(r.getRelationForest()), Share(r.getRelationDD())
  );

  // oldr := emptyset
  r.getStateForest()->buildSymbolicConst(false, oldr);

  // build set of deadlocked states satisfying p
  mdl->findDeadlockedStates(pdead);

#ifdef DEBUG_EG
  DisplayStream mycout(stdout);
  Show(mycout, "pdead: ", pdead);
#endif

  // r := p
  r.getStateForest()->copyEdge(p.getStateDD(), r.changeStateDD());
  
  while (!oldr->Equals(r.getStateDD())) {
    // f := preimage(r, R)
    r.getStateForest()->preImage(
            r.getStateDD(), r.getRelationDD(), f
    );

    // f := f + pdead
    r.getStateForest()->buildAssoc(
            f, false, exprman::aop_or, pdead.getStateDD(), f
    );

    // oldr := r
    r.getStateForest()->copyEdge(r.getStateDD(), oldr);

    // r := f * p
    r.getStateForest()->buildAssoc(
            f, false, exprman::aop_and, p.getStateDD(), r.changeStateDD()
    );
  } // while

  // Cleanup
  Delete(oldr);
  Delete(f);
}


// **************************************************************************
// *                                                                        *
// *                           CTL_symb_eng class                           *
// *                                                                        *
// **************************************************************************

/// Abstract base class for symbolic CTL engines.
class CTL_symb_eng : public subengine {
public:
  CTL_symb_eng();
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  inline static void convert(sv_encoder::error e) {
    switch (e) {
      case sv_encoder::Out_Of_Memory  :   throw Out_Of_Memory;
      default                         :   throw Engine_Failed;
    };
    throw Engine_Failed;
  }
};

CTL_symb_eng::CTL_symb_eng() : subengine()
{
}

bool CTL_symb_eng::AppliesToModelType(hldsm::model_type mt) const
{
  return (0==mt); 
}


// **************************************************************************
// *                                                                        *
// *                           EX_symb_eng  class                           *
// *                                                                        *
// **************************************************************************

class EX_symb_eng : public CTL_symb_eng {
public:
  EX_symb_eng();
  virtual void RunEngine(result* pass, int np, traverse_data &x);
};

EX_symb_eng the_EX_symb_eng;

EX_symb_eng::EX_symb_eng() : CTL_symb_eng()
{
}

void EX_symb_eng::RunEngine(result* pass, int np, traverse_data &x)
{
  DCASSERT(2==np);
  DCASSERT(pass[0].isNormal());
  DCASSERT(pass[1].isNormal());
  stateset* p = smart_cast <stateset*> (pass[1].getPtr());
  DCASSERT(p);
  // const checkable_lldsm* mdl = 
    // smart_cast <const checkable_lldsm*>(p->getParent());
  // DCASSERT(mdl);

  shared_object* rdd = 0;
  try {
    rdd = p->getStateForest()->makeEdge(0);
    if (pass[0].getBool()) {
      p->getStateForest()->postImage(p->getStateDD(), p->getRelationDD(), rdd);
    } else {
      p->getStateForest()->preImage(p->getStateDD(), p->getRelationDD(), rdd);
    }
    x.answer->setPtr(
      new stateset(
        p->getParent(), Share(p->getStateForest()), rdd,
        Share(p->getRelationForest()), Share(p->getRelationDD())
      )
    );
  }
  catch (sv_encoder::error err) {
    Delete(rdd);
    x.answer->setNull();
    convert(err);
  }
}

// **************************************************************************
// *                                                                        *
// *                           EU_symb_eng  class                           *
// *                                                                        *
// **************************************************************************

class EU_symb_eng : public CTL_symb_eng {
public:
  EU_symb_eng();
  virtual void RunEngine(result* pass, int np, traverse_data &x);
};

EU_symb_eng the_EU_symb_eng;

EU_symb_eng::EU_symb_eng() : CTL_symb_eng()
{
}

void EU_symb_eng::RunEngine(result* pass, int np, traverse_data &x)
{
  DCASSERT(3==np);
  DCASSERT(pass[0].isNormal());
  DCASSERT(pass[2].isNormal());
  
  stateset* q = smart_cast <stateset*> (pass[2].getPtr());
  DCASSERT(q);
  DCASSERT(q->isSymbolic());
  const checkable_lldsm* mdl = 
    smart_cast <const checkable_lldsm*>(q->getParent());
  stateset* p = 0;

  if (pass[1].isNormal()) {
    p = smart_cast <stateset*> (pass[1].getPtr());
    DCASSERT(p);
    DCASSERT(p->getParent() == mdl);
  }

  stateset* r = new stateset(
    q->getParent(), 
    Share(q->getStateForest()), 
    q->getStateForest()->makeEdge(0),
    Share(q->getRelationForest()), 
    Share(q->getRelationDD())
  );
  x.answer->setPtr(r);
  try {
    if (p) {
      if (pass[0].getBool()) {
        symbolic_ES(mdl, *p, *q, *r);
      } else {
        symbolic_EU(mdl, *p, *q, *r);
      }
    } else {
      if (pass[0].getBool()) {
        q->getStateForest()->postImageStar(
                q->getStateDD(), q->getRelationDD(), r->changeStateDD()
        );
      } else {
        q->getStateForest()->preImageStar(
                q->getStateDD(), q->getRelationDD(), r->changeStateDD()
        );
      }
    }
  }
  catch (sv_encoder::error err) {
    x.answer->setNull();
    convert(err);
  }
}

// **************************************************************************
// *                                                                        *
// *                        unfairEG_symb_eng  class                        *
// *                                                                        *
// **************************************************************************

class unfairEG_symb_eng : public CTL_symb_eng {
public:
  unfairEG_symb_eng();
  virtual void RunEngine(result* pass, int np, traverse_data &x);
};

unfairEG_symb_eng the_unfairEG_symb_eng;

unfairEG_symb_eng::unfairEG_symb_eng() : CTL_symb_eng()
{
}

void unfairEG_symb_eng::RunEngine(result* pass, int np, traverse_data &x)
{
  DCASSERT(2==np);
  DCASSERT(pass[0].isNormal());
  DCASSERT(pass[1].isNormal());
  stateset* p = smart_cast <stateset*> (pass[1].getPtr());
  DCASSERT(p);
  DCASSERT(p->isSymbolic());
  const checkable_lldsm* mdl = 
    smart_cast <const checkable_lldsm*>(p->getParent());
  DCASSERT(mdl);

  stateset* r = new stateset(
    p->getParent(), 
    Share(p->getStateForest()), 
    p->getStateForest()->makeEdge(0),
    Share(p->getRelationForest()), 
    Share(p->getRelationDD())
  );
  x.answer->setPtr(r);

  try {
    if (pass[0].getBool()) {
      unfair_EH(mdl, *p, *r);
    } else {
      unfair_EG(mdl, *p, *r);
    }
  }
  catch (sv_encoder::error err) {
    x.answer->setNull();
    convert(err);
  }
}

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeSymbolicCTLEngines(exprman* em)
{
  if (0==em) return;

  RegisterEngine(
      em,
      "SymbolicEX",
      "traditional", 
      "Use traditional pre-image operation",
      &the_EX_symb_eng
  );

  RegisterEngine(
      em,
      "SymbolicEU",
      "Traditional",
      "Use traditional pre-image, intersection iterations",
      &the_EU_symb_eng
  );

  RegisterEngine(
      em,
      "SymbolicUnfairEG",
      "Traditional", 
      "Use traditional pre-image, intersection iterations",
      &the_unfairEG_symb_eng 
  );
}

