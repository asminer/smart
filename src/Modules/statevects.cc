
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

#include "statevects.h"

// ******************************************************************
// *                                                                *
// *                       expl_printer class                       *
// *                                                                *
// ******************************************************************

class sparse_vect_printer : public lldsm::state_visitor {
  OutputStream &out;
  const double* dist;
  bool print_indexes;
  bool comma;
public:
  sparse_vect_printer(const hldsm* mdl, OutputStream &s, const double* d, bool pi);
  virtual bool canSkipIndex();
  virtual bool visit();
};

sparse_vect_printer
::sparse_vect_printer(const hldsm* m, OutputStream &s, const double* d, bool pi)
 : state_visitor(m), out(s)
{
  dist = d;
  print_indexes = pi;
  comma = false;
}

bool sparse_vect_printer::canSkipIndex()
{
  return (0==dist[x.current_state_index] );
}

bool sparse_vect_printer::visit()
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
// *                       statevect  methods                       *
// *                                                                *
// ******************************************************************

int statevect::display_style;

statevect::statevect(const stochastic_lldsm* p, const double* d, long N)
: shared_object()
{
  parent = p;
  vect = new double[N];
  memcpy(vect, d, N*sizeof(double));
  numStates = N;
}

statevect::~statevect()
{
  // printf("Destroying state vector\n");  
  delete[] vect;
}

bool statevect::Print(OutputStream &s, int width) const
{
  DCASSERT(parent);
  DCASSERT(vect);
  if (FULL==display_style) {
    s.Put('[');
    s << vect[0];
    for (int i=1; i<numStates; i++) {
      s << ", " << vect[i];
    }
    s.Put(']');
  } else {
    sparse_vect_printer foo(parent->GetParent(), s, vect, SINDEX==display_style);
    s.Put('(');
    parent->visitStates(foo);
    s.Put(')');
  }
  return true;
}

bool statevect::Equals(const shared_object *o) const
{
  const statevect* b = dynamic_cast <const statevect*> (o);
  if (0==b) return false;
  if (parent != b->parent) return false;  // TBD: may want to allow this
  
  DCASSERT(numStates == b->numStates);

  for (long i=0; i<numStates; i++) {
    if (vect[i] != b->vect[i]) return false;
    // TBD - check within epsilon?
  }
  return true;
}

// ******************************************************************
// *                                                                *
// *                       statedist  methods                       *
// *                                                                *
// ******************************************************************

statedist::statedist(const stochastic_lldsm *p, const double *d, long N)
 : statevect(p, d, N)
{
}

// ******************************************************************
// *                                                                *
// *                       stateprobs methods                       *
// *                                                                *
// ******************************************************************

stateprobs::stateprobs(const stochastic_lldsm *p, const double *d, long N)
 : statevect(p, d, N)
{
}

// ******************************************************************
// *                                                                *
// *                       statemsrs  methods                       *
// *                                                                *
// ******************************************************************

statemsrs::statemsrs(const stochastic_lldsm *p, const double *d, long N)
 : statevect(p, d, N)
{
}

// ******************************************************************
// *                                                                *
// *                      statedist_type class                      *
// *                                                                *
// ******************************************************************

class statedist_type : public simple_type {
public:
  statedist_type();
};

statedist_type::statedist_type() : simple_type("statedist", "Distribution over states", "A probability distribution over a set of model states.  Elements will sum to one.")
{
  setPrintable();
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

stateprobs_type::stateprobs_type() : simple_type("stateprobs", "Probabilities for states", "A vector of probabilities over states, specifying a probability value for each state.  Note that this is not a distribution, but rather a special case of measures for each state, so probabilities may not sum to one.")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                      statemsrs_type class                      *
// *                                                                *
// ******************************************************************

class statemsrs_type : public simple_type {
public:
  statemsrs_type();
};

statemsrs_type::statemsrs_type() : simple_type("statemsrs", "Measures for states", "A vector of real--valued measures over states, specifying a measure for each state.")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// *                                                                *
// ******************************************************************


void InitStatevects(exprman* em, symbol_table* st)
{
  if (0==em)  return;
  
  // Type registry
  simple_type* t_statedist = new statedist_type;
  em->registerType(t_statedist);

  simple_type* t_stateprobs = new stateprobs_type;
  em->registerType(t_stateprobs);

  simple_type* t_statemsrs = new statemsrs_type;
  em->registerType(t_statemsrs);

  em->setFundamentalTypes();

  // Operators

  // Options
  // ------------------------------------------------------------------
  radio_button** style = new radio_button*[3];
  style[statevect::FULL] = new radio_button(
      "FULL",
      "Vectors are displayed in (raw) full.",
      statevect::FULL
  );
  style[statevect::SINDEX] = new radio_button(
      "SPARSE_INDEX",
      "Vectors are displayed in sparse format, with state indexes.",
      statevect::SINDEX
  );
  style[statevect::SSTATE] = new radio_button(
      "SPARSE_STATE",
      "Vectors are displayed in sparse format, with states.",
      statevect::SSTATE
  );
  statevect::display_style = statevect::SINDEX; 
  em->addOption(
    MakeRadioOption("StatevectDisplayStyle",
      "Style to display a statedist, stateprobs, or statemsrs vector.",
      style, 3, statevect::display_style
    )
  );

  if (0==st) return;

  // Functions
}

