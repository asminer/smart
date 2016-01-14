
// $Id$

#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"
#include "../Formlsms/graph_llm.h"

#include "expl_ssets.h"
#include "biginttype.h"

// external library
#include "intset.h"


// ******************************************************************
// *                                                                *
// *                       expl_printer class                       *
// *                                                                *
// ******************************************************************

class expl_printer : public state_lldsm::state_visitor {
  OutputStream &out;
  const intset &toprint;
  bool print_indexes;
  bool comma;
public:
  expl_printer(const hldsm* mdl, OutputStream &s, const intset &p, bool pi);
  virtual bool canSkipIndex();
  virtual bool visit();
};

expl_printer
::expl_printer(const hldsm* m, OutputStream &s, const intset &p, bool pi)
 : state_visitor(m), out(s), toprint(p)
{
  print_indexes = pi;
  comma = false;
}

bool expl_printer::canSkipIndex()
{
  return (! toprint.contains(x.current_state_index) );
}

bool expl_printer::visit()
{
  if (comma)  out << ", ";
  else        comma = true;
  if (print_indexes) {
    out.Put(x.current_state_index); 
  } else {
    x.current_state->Print(out, 0);
  }
  out.can_flush();  // otherwise, huge sets will overflow the buffer
  return false;
}

// ******************************************************************
// *                                                                *
// *                     expl_stateset  methods                     *
// *                                                                *
// ******************************************************************

expl_stateset::expl_stateset(const state_lldsm* p, intset* e) : stateset(p)
{
  data = e;
}

expl_stateset::~expl_stateset()
{
  delete data;
}

stateset* expl_stateset::DeepCopy() const
{
  DCASSERT(data);
  return new expl_stateset(getParent(), new intset (*data) );
}

bool expl_stateset::Complement() 
{
  if (0==data) return false;
  data->complement();
  return true;
}

bool expl_stateset::Union(const expr* c, const char* op, const stateset* x)
{
  if (0==data) return false;
  const expl_stateset* ex = dynamic_cast <const expl_stateset*> (x);
  if (0==ex) {
    storageMismatchError(c, op);
    return false;
  }

  (*data) += *(ex->data); 
  return true;
}

bool expl_stateset::Intersect(const expr* c, const char* op, const stateset* x)
{
  if (0==data) return false;
  const expl_stateset* ex = dynamic_cast <const expl_stateset*> (x);
  if (0==ex) {
    storageMismatchError(c, op);
    return false;
  }

  (*data) *= *(ex->data); 
  return true;
}

void expl_stateset::getCardinality(long &card) const
{
  DCASSERT(data);
  card = data->cardinality();
}

void expl_stateset::getCardinality(result &x) const
{
  DCASSERT(data);
  x.setPtr(new bigint(data->cardinality()));
}

bool expl_stateset::isEmpty() const
{
  DCASSERT(data);
  return data->isEmpty();
}

bool expl_stateset::Print(OutputStream &s, int) const
{
  expl_printer foo(getGrandparent(), s, *data, printIndexes());
  s.Put('{');
  getParent()->visitStates(foo);
  s.Put('}');
  return true;
}

bool expl_stateset::Equals(const shared_object *o) const
{
  const expl_stateset* b = dynamic_cast <const expl_stateset*> (o);
  if (0==b) return false;
  // TBD : may want to allow comparisons with other implementations

  if (getParent() != b->getParent()) return false;  // TBD: may want to allow this

  // Not sure if data can ever be 0, but this is probably 
  // the correct way to handle it if it is possible.
  if (0==data && 0==b->data) return true; 
  if (0==data || 0==b->data) return false;
  
  return (*data) == *(b->data);
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

intset_lib::intset_lib() : library(false)
{
}

const char* intset_lib::getVersionString() const
{
  return intset::getVersion();
}

intset_lib intset_lib_data;

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitExplStatesets(exprman* em)
{
  if (0==em)  return;
  
  // Library registry
  em->registerLibrary(  &intset_lib_data );
}

