
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
// *                     expl_tri_set_stateset  methods             *
// *                                                                *
// ******************************************************************

expl_tri_set_stateset::expl_tri_set_stateset(const state_lldsm* p, intset* t, intset* f) : set_stateset(p)
{
  trueset = new expl_stateset(p,t);
  falseset = new expl_stateset(p,f);
}

expl_tri_set_stateset::expl_tri_set_stateset(const state_lldsm* p, expl_stateset* t, expl_stateset* f) : set_stateset(p)
{
  trueset = t;
  falseset = f;
}

expl_tri_set_stateset::~expl_tri_set_stateset()
{
  // delete trueset;
  // delete falseset;
}

set_stateset* expl_tri_set_stateset::DeepCopy() const
{
  DCASSERT(trueset);
  DCASSERT(falseset);
  return new expl_tri_set_stateset(getParent(), trueset->DeepCopy(), falseset->DeepCopy() );
}

bool expl_tri_set_stateset::Complement() 
{
  expl_stateset* tmp = trueset;
  trueset = falseset;
  falseset = tmp;
  return true;
}

bool expl_tri_set_stateset::Union(const expr* c, const char* op, const set_stateset* x)
{
  if (0==trueset || 0==falseset) return false;
  const expl_tri_set_stateset* ex = dynamic_cast <const expl_tri_set_stateset*> (x);
  if (0==ex) {
    storageMismatchError(c, op);
    return false;
  }

  trueset->Union(c,op,ex->trueset);
  falseset->Intersect(c,op,ex->falseset); // verify
  return true;
}

bool expl_tri_set_stateset::Intersect(const expr* c, const char* op, const set_stateset* x)
{
  if (0==trueset || 0==falseset) return false;
  const expl_tri_set_stateset* ex = dynamic_cast <const expl_tri_set_stateset*> (x);
  if (0==ex) {
    storageMismatchError(c, op);
    return false;
  }

  trueset->Intersect(c,op,ex->trueset);
  falseset->Intersect(c,op,ex->falseset);
  return true;
}

bool expl_tri_set_stateset::Plus(const expr* c, const char* op, const set_stateset* x)
{
  return Intersect(c, op, x);
}
  
void expl_tri_set_stateset::getTrueCardinality(long &card) const
{
  DCASSERT(trueset);
  trueset->getCardinality(card);
}

void expl_tri_set_stateset::getTrueCardinality(result &x) const
{
  DCASSERT(trueset);
  trueset->getCardinality(x);
}

void expl_tri_set_stateset::getFalseCardinality(long &card) const
{
  DCASSERT(falseset);
  falseset->getCardinality(card);
}

void expl_tri_set_stateset::getFalseCardinality(result &x) const
{
  DCASSERT(falseset);
  falseset->getCardinality(x);
}

void expl_tri_set_stateset::getUnknownCardinality(long &card) const
{
  DCASSERT(trueset);
  DCASSERT(falseset);
  // ???
}

void expl_tri_set_stateset::getUnknownCardinality(result &x) const
{
  DCASSERT(trueset);
  DCASSERT(falseset);
  // ???
}

bool expl_tri_set_stateset::isTrueEmpty() const
{
  DCASSERT(trueset);
  return trueset->isEmpty();
}

bool expl_tri_set_stateset::isFalseEmpty() const
{
  DCASSERT(falseset);
  return falseset->isEmpty();
}

bool expl_tri_set_stateset::Print(OutputStream &s, int) const
{
  s.Put('True {');
  trueset->Print(s,0);
  s.Put('}');
  s.Put('False {');
  falseset->Print(s,0);
  s.Put('}');
  // s.Put('Unknown set {');
  // falseset->Print(s,0);
  // s.Put('}');
  return true;
}

bool expl_tri_set_stateset::Equals(const shared_object *o) const
{
  const expl_tri_set_stateset* b = dynamic_cast <const expl_tri_set_stateset*> (o);
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


// ******************************************************************
// *                                                                *
// *                     intset library credits                     *
// *                                                                *
// ******************************************************************

class intset_lib : public library {
public:
  intset_lib();
  virtual const char* getVersionString() const;
  virtual bool hasFixedPointer() const { return true; }
};

intset_lib::intset_lib() : library(false, false)
{
}

const char* intset_lib::getVersionString() const
{
  return intset::getVersion();
}

intset_lib intset_lib_data;

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_expltrissets : public initializer {
  public:
    init_expltrissets();
    virtual bool execute();
};
init_expltrissets the_explsset_initializer;

init_expltrissets::init_expltrissets() : initializer("init_expltrissets")
{
  usesResource("em");
}

bool init_expltrissets::execute()
{
  if (0==em)  return false;
  
  // Library registry
  em->registerLibrary(  &intset_lib_data );
  return true;
}

