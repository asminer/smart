
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"
#include "../Formlsms/graph_llm.h"

#include "expl_ssets.h"
#include "biginttype.h"

// external library
#include "../_IntSets/intset.h"

#include "expl_trissets.h"

// ******************************************************************
// *                                                                *
// *                     expl_tri_stateset  methods                 *
// *                                                                *
// ******************************************************************

expl_tri_stateset::expl_tri_stateset(const state_lldsm* p, intset* t, intset* f) : stateset(p)
{
  trueset = new expl_stateset(p,t);
  falseset = new expl_stateset(p,f);
}

expl_tri_stateset::expl_tri_stateset(const state_lldsm* p, expl_stateset* t, expl_stateset* f) : stateset(p)
{
  trueset = t;
  falseset = f;
}

expl_tri_stateset::expl_tri_stateset(const state_lldsm* p, stateset* t, stateset* f) :  stateset(p)
{
  trueset = dynamic_cast<expl_stateset*>(t);
  falseset = dynamic_cast<expl_stateset*>(f);
}

expl_tri_stateset::expl_tri_stateset(const state_lldsm* p, stateset* t) :  stateset(p)
{
  trueset = dynamic_cast<expl_stateset*>(t);
  if (trueset) {
    falseset = trueset->DeepCopy();
    falseset->Complement();
  } else {
    expl_tri_stateset* tmp = dynamic_cast<expl_tri_stateset*>(t);
    trueset = tmp->trueset->DeepCopy();
    falseset = tmp->falseset->DeepCopy();
  }
}

expl_tri_stateset::expl_tri_stateset(const state_lldsm* p, const expl_stateset* t) : stateset(p)
{
  trueset = t->DeepCopy();
  falseset = t->DeepCopy();
  falseset->Complement();
}


expl_tri_stateset::~expl_tri_stateset()
{
  // delete trueset;
  // delete falseset;
}

expl_tri_stateset* expl_tri_stateset::DeepCopy() const
{
  DCASSERT(trueset);
  DCASSERT(falseset);
  return new expl_tri_stateset(getParent(), trueset->DeepCopy(), falseset->DeepCopy() );
}

expl_stateset* expl_tri_stateset::computeUnknownSet() const {
  intset t = trueset->getExplicit();
  intset f = falseset->getExplicit();

  intset u = !(t+f);

  return new expl_stateset(this->getParent(), new intset(u));
};

bool expl_tri_stateset::Complement() 
{
  expl_stateset* tmp = trueset;
  trueset = falseset;
  falseset = tmp;
  return true;
}

#include <iostream>
bool expl_tri_stateset::Union(const expr* c, const char* op, const stateset* x)
{
  if (0==trueset || 0==falseset) return false;
  const expl_tri_stateset* ext = dynamic_cast <const expl_tri_stateset*> (x);
  if (0==ext) {
    const expl_stateset* ex = dynamic_cast <const expl_stateset*> (x);
    if (0==ex) {
      storageMismatchError(c, op);
      return false;
    } else {
      trueset->Union(c,op,ex);
      expl_stateset* copy = ex->DeepCopy();
      copy->Complement();
      falseset->Intersect(c,op,copy);
      return true;
    }
  } else {
    trueset->Union(c,op,ext->trueset);
    falseset->Intersect(c,op,ext->falseset);
    return true;
  }
}

bool expl_tri_stateset::Intersect(const expr* c, const char* op, const stateset* x)
{
  if (0==trueset || 0==falseset) return false;
  const expl_tri_stateset* ext = dynamic_cast <const expl_tri_stateset*> (x);
  if (0==ext) {
    const expl_stateset* ex = dynamic_cast <const expl_stateset*> (x);
    if (0==ex) {
      storageMismatchError(c, op);
      return false;
    } else {
      trueset->Intersect(c,op,ex);
      expl_stateset* copy = ex->DeepCopy();
      copy->Complement();
      falseset->Intersect(c,op,copy);
      return true;
    }
  } else {
    trueset->Intersect(c,op,ext->trueset);
    falseset->Intersect(c,op,ext->falseset);
    return true;
  }
}

bool expl_tri_stateset::Plus(const expr* c, const char* op, const stateset* x)
{
  return Intersect(c, op, x);
}

void expl_tri_stateset::getCardinality(long &card) const
{
  DCASSERT(false);
}

void expl_tri_stateset::getCardinality(result &x) const
{
  DCASSERT(false);
}
  
void expl_tri_stateset::getTrueCardinality(long &card) const
{
  DCASSERT(trueset);
  trueset->getCardinality(card);
}

void expl_tri_stateset::getTrueCardinality(result &x) const
{
  DCASSERT(trueset);
  trueset->getCardinality(x);
}

void expl_tri_stateset::getFalseCardinality(long &card) const
{
  DCASSERT(falseset);
  falseset->getCardinality(card);
}

void expl_tri_stateset::getFalseCardinality(result &x) const
{
  DCASSERT(falseset);
  falseset->getCardinality(x);
}

void expl_tri_stateset::getUnknownCardinality(long &card) const
{
  DCASSERT(trueset);
  DCASSERT(falseset);
  // ???
}

void expl_tri_stateset::getUnknownCardinality(result &x) const
{
  DCASSERT(trueset);
  DCASSERT(falseset);
  // ???
}

bool expl_tri_stateset::isEmpty() const
{
  // TODO: error
  return false; 
}

bool expl_tri_stateset::isTrueEmpty() const
{
  DCASSERT(trueset);
  return trueset->isEmpty();
}

bool expl_tri_stateset::isFalseEmpty() const
{
  DCASSERT(falseset);
  return falseset->isEmpty();
}

bool expl_tri_stateset::Print(OutputStream &s, int) const
{
  s.Put('T');
  s.Put('{');
  trueset->Print(s,0);
  s.Put('}');
  s.Put('F');
  s.Put('{');
  falseset->Print(s,0);
  s.Put('}');
  // s.Put('Unknown set {');
  // falseset->Print(s,0);
  // s.Put('}');
  return true;
}

bool expl_tri_stateset::Equals(const shared_object *o) const
{
  const expl_tri_stateset* b = dynamic_cast <const expl_tri_stateset*> (o);
  if (0==b) return false;
  // TBD : may want to allow comparisons with other implementations

  if (getParent() != b->getParent()) return false;  // TBD: may want to allow this

  // Not sure if data can ever be 0, but this is probably 
  // the correct way to handle it if it is possible.
  if (0==trueset && 0==b->trueset) return true; 
  if (0==trueset || 0==b->trueset) return false;
  if (0==falseset && 0==b->falseset) return true; 
  if (0==falseset || 0==b->falseset) return false;
  
  return trueset->Equals(b->trueset) && falseset->Equals(b->falseset); 
}

