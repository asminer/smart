
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
  : parent(nullptr)
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

void trace::setParent(const trace* t)
{
  DCASSERT(nullptr == parent);
  parent = t;
}

int trace::Length() const
{
  return states.Length();
}

int trace::TotalLength() const
{
  int length = Length();
  for (int i = 0; i < subtraces.Length(); i++) {
    const trace* st = subtraces.ReadItem(i);
    if (nullptr != st) {
      length += st->TotalLength() - 1;
    }
  }
  return length;
}

void trace::Append(shared_state* state)
{
  states.Append(state);
  subtraces.Append(nullptr);
}

const shared_state* trace::getState(int i) const
{
  return states.ReadItem(i);
}

void trace::Concatenate(int i, trace* subtrace)
{
  if (nullptr != subtraces.ReadItem(i)) {
    Delete(const_cast<trace*>(subtraces.ReadItem(i)));
  }
  subtraces.Update(i, Share(const_cast<trace*>(subtrace)));
  subtrace->setParent(this);
}

const trace* trace::getSubtrace(int i) const
{
  return subtraces.ReadItem(i);
}

bool trace::Print(OutputStream &s, int width) const
{
  s << "\n";
  int total = TotalLength();
  if (nullptr == parent) {
    s << "Length: " << total << "\n";
  }

  if (total <= 100) {
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
