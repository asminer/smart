
#include "../Options/options.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"

#include "../Formlsms/graph_llm.h"

#include "trace.h"

// ******************************************************************
// *                                                                *
// *                         trace methods                          *
// *                                                                *
// ******************************************************************

exprman* trace::em = 0;

trace::trace()
{
}

trace::~trace()
{
  for (int i = 0; i < states.Length(); i++) {
    Delete(states.Item(i));
  }
  for (int i = 0; i < subtraces.Length(); i++) {
    Delete(subtraces.Item(i));
  }
}

int trace::Length() const
{
  int length = states.Length();
  for (int i = 0; i < subtraces.Length(); i++) {
    const trace* st = subtraces.ReadItem(i);
    if (nullptr != st) {
      length += st->Length();
    }
  }
  return length;
}

void trace::Append(const shared_state* state)
{
  states.Append(Share(const_cast<shared_state*>(state)));
  subtraces.Append(nullptr);
}

const shared_state* trace::getState(int i) const
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

bool trace::Print(OutputStream &s, int width) const
{
  s << "\n";
  for (int i = 0; i < width; i++) {
    s << " ";
  }
  for (int i = 0; i < states.Length(); i++) {
    if (i != 0) {
      s << " -> ";
    }
    states.ReadItem(i)->Print(s, width);
  }

  for (int i = 0; i < subtraces.Length(); i++) {
    const trace* st = subtraces.ReadItem(i);
    if (nullptr != st) {
      subtraces.ReadItem(i)->Print(s, width + 3);
    }
  }
  return true;
}

bool trace::Equals(const shared_object *o) const
{
  DCASSERT(0);
  return false;
}

// ******************************************************************
// *                                                                *
// *                       trace_data  class                        *
// *                                                                *
// ******************************************************************

trace_data::trace_data()
{
}

bool trace_data::Print(OutputStream &s, int width) const
{
  // TODO: To be implemented
  return true;
}

bool trace_data::Equals(const shared_object *o) const
{
  DCASSERT(0);
  return false;
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
