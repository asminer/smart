
// $Id$

#include "../Options/options.h"
#include "../ExprLib/exprman.h"
// #include "../ExprLib/unary.h"
// #include "../ExprLib/binary.h"
// #include "../ExprLib/assoc.h"
// #include "../SymTabs/symtabs.h"
// #include "../ExprLib/functions.h"
// #include "biginttype.h"
#include "../ExprLib/mod_vars.h"
// #include "../ExprLib/dd_front.h"

#include "../Formlsms/stoch_llm.h"

#include "stateprobs.h"

// ******************************************************************
// *                                                                *
// *                       expl_printer class                       *
// *                                                                *
// ******************************************************************

class sprobs_expl_printer : public lldsm::state_visitor {
  OutputStream &out;
  const double* dist;
  bool print_indexes;
  bool comma;
public:
  sprobs_expl_printer(const hldsm* mdl, OutputStream &s, const double* d, bool pi);
  virtual bool canSkipIndex();
  virtual bool visit();
};

sprobs_expl_printer
::sprobs_expl_printer(const hldsm* m, OutputStream &s, const double* d, bool pi)
 : state_visitor(m), out(s)
{
  dist = d;
  print_indexes = pi;
  comma = false;
}

bool sprobs_expl_printer::canSkipIndex()
{
  return (0==dist[x.current_state_index] );
}

bool sprobs_expl_printer::visit()
{
  if (comma)  out << ", ";
  else        comma = true;
  if (print_indexes) {
    out.Put(x.current_state_index); 
  } else {
    x.current_state->Print(out, 0);
  }
  out << ":" << dist[x.current_state_index];
  return false;
}

// ******************************************************************
// *                                                                *
// *                       stateprobs methods                       *
// *                                                                *
// ******************************************************************

bool stateprobs::print_indexes;

stateprobs::stateprobs(const stochastic_lldsm* p, double* d, long N)
: shared_object()
{
  parent = p;
  dist = d;
  numStates = N;
}

stateprobs::~stateprobs()
{
  delete[] dist;
}

bool stateprobs::Print(OutputStream &s, int width) const
{
  DCASSERT(parent);
  DCASSERT(dist);
  sprobs_expl_printer foo(parent->GetParent(), s, dist, print_indexes);
  s.Put('{');
  parent->visitStates(foo);
  s.Put('}');
  return true;
}

bool stateprobs::Equals(const shared_object *o) const
{
  const stateprobs* b = dynamic_cast <const stateprobs*> (o);
  if (0==b) return false;
  if (parent != b->parent) return false;  // TBD: may want to allow this
  
  DCASSERT(numStates == b->numStates);

  for (long i=0; i<numStates; i++) {
    if (dist[i] != b->dist[i]) return false;
    // TBD - check within epsilon?
  }
  return true;
}

// ******************************************************************
// *                                                                *
// *                     stateprobs_type  class                     *
// *                                                                *
// ******************************************************************

class stateprobs_type : public simple_type {
public:
  stateprobs_type();
};

// ******************************************************************
// *                    stateprobs_type  methods                    *
// ******************************************************************

stateprobs_type::stateprobs_type() : simple_type("stateprobs")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                     stateprobs expressions                     *
// *                                                                *
// *                                                                *
// ******************************************************************



// ******************************************************************
// *                                                                *
// *                                                                *
// *                     stateprobs  operations                     *
// *                                                                *
// *                                                                *
// ******************************************************************


// ******************************************************************
// *                                                                *
// *                                                                *
// *                           Functions                            *
// *                                                                *
// *                                                                *
// ******************************************************************



// ******************************************************************
// *                                                                *
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// *                                                                *
// ******************************************************************


void InitStateprobs(exprman* em, symbol_table* st)
{
  if (0==em)  return;
  
  // Type registry
  simple_type* t_stateprobs = new stateprobs_type;
  em->registerType(t_stateprobs);
  em->setFundamentalTypes();

  // Operators

  // Options
  stateprobs::print_indexes = true;
  em->addOption(
    MakeBoolOption("StateprobsPrintIndexes", 
      "If true, when a stateprobs vector is printed, state indexes are displayed; otherwise, states are displayed.",
      stateprobs::print_indexes
    )
  );

  if (0==st) return;

  // Functions
}

