
/** \file trace.h

    Module for traces.
    Defines a trace type, and appropriate operators.
*/

#ifndef TRACE_H
#define TRACE_H

#include "../include/shared.h"
#include "statesets.h"

// ******************************************************************
// *                                                                *
// *                          trace class                           *
// *                                                                *
// ******************************************************************

/**
    Abstract base class for trace.
    Derived classes for different implementations.
    Any expression of type trace will build an object of this type
    (or derived class, of course).
*/
class trace : public shared_object {
public:
  trace();

protected:
  virtual ~trace();

public:
  virtual int Length() const;
  virtual void Append(const shared_state* state);
  virtual const shared_state* getState(int i) const;
  virtual void Concatenate(int i, const trace* subtrace);
  virtual const trace* getSubtrace(int i) const;

  virtual bool Print(OutputStream &s, int width) const;
  virtual bool Equals(const shared_object *o) const;

private:
  static exprman* em;
  List<shared_state> states;
  List<trace> subtraces;

  friend class init_trace;
};

#endif
