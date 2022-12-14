
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
// *                       expl_printer class                       *
// *                                                                *
// ******************************************************************

class expl_printer : public state_lldsm::state_visitor {
  std::ostream &out;
  const intset &toprint;
  bool print_indexes;
  bool comma;
public:
  expl_printer(const hldsm* mdl, std::ostream &s, const intset &p, bool pi);
  virtual bool canSkipIndex();
  virtual bool visit();
};

expl_printer
::expl_printer(const hldsm* m, std::ostream &s, const intset &p, bool pi)
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
    out << x.current_state_index;
  } else {
    x.current_state->Print(out);
  }
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

bool expl_stateset::Plus(const expr* c, const char* op, const stateset* x)
{
  return Intersect(c, op, x);
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

bool expl_stateset::Print(std::ostream &s, int) const
{
  expl_printer foo(getGrandparent(), s, *data, printIndexes());
  s << '{';
  getParent()->visitStates(foo);
  s << '}';
  return true;
}

int expl_stateset::Compare(const shared_object *o) const
{
  const expl_stateset* b = dynamic_cast <const expl_stateset*> (o);
  DCASSERT(b);
  // TBD : may want to allow comparisons with other implementations

  DCASSERT(getParent() == b->getParent());
  // TBD: may want to allow this

  // Not sure if data can ever be 0, but this is probably
  // the correct way to handle it if it is possible
  // (assuming 0 means empty set).
  if (!data && !b->data) return 0;
  if (!data) return -1;
  if (!b->data) return +1;

  return data->compare(*(b->data));
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

class init_explssets : public startup {
  public:
    init_explssets();
    virtual bool execute();
};
init_explssets the_explsset_startup;

init_explssets::init_explssets() : startup("init_explssets")
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

