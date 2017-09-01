
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
  trace(const state_lldsm* p);
  trace(const trace* clone);

protected:
  virtual ~trace();

public:
  const state_lldsm* getParent() const { return parent; }

  virtual int Length() const;
  virtual void Append(const stateset* state);
  virtual const stateset* getState(int i) const;
  virtual void Concatenate(int i, const trace* subtrace);
  virtual const trace* getSubtrace(int i) const;

private:
  static exprman* em;
  const state_lldsm* parent;
  List<stateset> states;
  List<trace> subtraces;

  friend class init_trace;
};

#endif
