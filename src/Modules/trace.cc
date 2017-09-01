
#include "../Options/options.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../SymTabs/symtabs.h"

#include "../Formlsms/graph_llm.h"

#include "trace.h"

// ******************************************************************
// *                                                                *
// *                         trace methods                          *
// *                                                                *
// ******************************************************************

exprman* trace::em = 0;

trace::trace(const state_lldsm* p) : shared_object()
{
  parent = p;
}

trace::trace(const trace* clone) : shared_object()
{
  DCASSERT(clone);
  parent = clone->parent;
}

trace::~trace()
{
  for (int i = 0; i < states.Length(); i++) {
    Delete(const_cast<stateset*>(states.ReadItem(i)));
  }
  for (int i = 0; i < subtraces.Length(); i++) {
    Delete(const_cast<trace*>(subtraces.ReadItem(i)));
  }
}

int trace::Length() const
{
  return states.Length();
}

void trace::Append(const stateset* state)
{
  states.Append(Share(const_cast<stateset*>(state)));
  subtraces.Append(nullptr);
}

const stateset* trace::getState(int i) const
{
  return states.ReadItem(i);
}

void trace::Concatenate(int i, const trace* subtrace)
{
  if (nullptr != subtraces.ReadItem(i)) {
    Delete(const_cast<trace*>(subtraces.ReadItem(i)));
  }
  subtraces.Update(i, Share(const_cast<trace*>(subtrace)));
}

const trace* trace::getSubtrace(int i) const
{
  return subtraces.ReadItem(i);
}

// ******************************************************************
// *                                                                *
// *                       trace_type  class                        *
// *                                                                *
// ******************************************************************

class trace_type : public simple_type {
public:
  trace_type();
};

// ******************************************************************
// *                      trace_type  methods                       *
// ******************************************************************

trace_type::trace_type()
  : simple_type("trace", "Tree of states", "Type used for tree of states, used for CTL witness generation.")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_trace : public initializer {
  public:
    init_trace();
    virtual bool execute();
};
init_trace the_trace_initializer;

init_trace::init_trace() : initializer("init_trace")
{
  usesResource("em");
  usesResource("st");
  buildsResource("tracetype");
  buildsResource("types");
}

bool init_trace::execute()
{
  if (0==em)  return false;

  trace::em = em;

  // Type registry
  simple_type* t_trace = new trace_type;
  em->registerType(t_trace);
  em->setFundamentalTypes();

  if (0==st) return false;

  return true;
}
