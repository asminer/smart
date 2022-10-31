
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"
#include "../Formlsms/graph_llm.h"

#include "expl_ssets.h"
#include "biginttype.h"

// external library
#include "../_IntSets/intset.h"


// ******************************************************************
// *                                                                *
// *                       expl_tri_printer class                   *
// *                                                                *
// ******************************************************************

class expl_tri_printer : public state_lldsm::state_visitor {
  OutputStream &out;
  const intset &toprint;
  bool print_indexes;
  bool comma;
public:
  expl_tri_printer(const hldsm* mdl, OutputStream &s, const intset &p, bool pi);
  virtual bool canSkipIndex();
  virtual bool visit();
};

expl_tri_printer
::expl_tri_printer(const hldsm* m, OutputStream &s, const intset &p, bool pi)
 : state_visitor(m), out(s), toprint(p)
{
  print_indexes = pi;
  comma = false;
}

bool expl_tri_printer::canSkipIndex()
{
  return (! toprint.contains(x.current_state_index) );
}

bool expl_tri_printer::visit()
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
// *                     expl_tri_set_stateset  methods                     *
// *                                                                *
// ******************************************************************

expl_tri_set_stateset::expl_tri_set_stateset(const state_lldsm* p, intset* t, intset* f) : set_stateset(p)
{
  data.push_back(t);
  data.push_back(f);
}

expl_tri_set_stateset::~expl_tri_set_stateset()
{
  delete true_data;
  delete false_data;
}

stateset* expl_tri_set_stateset::DeepCopy() const
{
  DCASSERT(data);
  return new expl_tri_set_stateset(getParent(), new intset (*true_data), new intset(*false_data) );
}

bool expl_tri_set_stateset::Complement() 
{
  intset* tmp = true_data;
  true_data = false_data;
  false_data = tmp;
  return true;
}

bool expl_tri_set_stateset::Union(const expr* c, const char* op, const stateset* x)
{
  if (0==data) return false;
  const expl_tri_set_stateset* ex = dynamic_cast <const expl_tri_set_stateset*> (x);
  if (0==ex) {
    storageMismatchError(c, op);
    return false;
  }

  (*true_data) += *(ex->true_data); 
  (*false_data) += *(ex->false_data);
  return true;
}

bool expl_tri_set_stateset::Intersect(const expr* c, const char* op, const stateset* x)
{
  if (0==data) return false;
  const expl_tri_set_stateset* ex = dynamic_cast <const expl_tri_set_stateset*> (x);
  if (0==ex) {
    storageMismatchError(c, op);
    return false;
  }

  (*true_data) *= *(ex->true_data); 
  (*false_data) *= *(ex->false_data); 
  return true;
}

bool expl_tri_set_stateset::Plus(const expr* c, const char* op, const stateset* x)
{
  return Intersect(c, op, x);
}

void expl_tri_set_stateset::getCardinality(long &card) const
{
  DCASSERT(true_data);
  DCASSERT(false_data);
  card = data->cardinality();
}

void expl_tri_set_stateset::getCardinality(result &x) const
{
  DCASSERT(data);
  x.setPtr(new bigint(data->cardinality()));
}

bool expl_tri_set_stateset::isEmpty() const
{
  DCASSERT(data);
  return data->isEmpty();
}

bool expl_tri_set_stateset::Print(OutputStream &s, int) const
{
  expl_tri_printer foo(getGrandparent(), s, *data, printIndexes());
  s.Put('{');
  getParent()->visitStates(foo);
  s.Put('}');
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

class init_explssets : public initializer {
  public:
    init_explssets();
    virtual bool execute();
};
init_explssets the_explsset_initializer;

init_explssets::init_explssets() : initializer("init_explssets")
{
  usesResource("em");
}

bool init_explssets::execute()
{
  if (0==em)  return false;
  
  // Library registry
  em->registerLibrary(  &intset_lib_data );
  return true;
}

