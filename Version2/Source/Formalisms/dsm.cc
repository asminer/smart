
// $Id$

#include "dsm.h"

#include "../States/reachset.h"
#include "../Base/memtrack.h"

/** @name dsm.cc
    @type File
    @args \ 

  Implementation for Discrete-state model "base class".

*/

//@{

void Delete(state_model *x) 
{
  delete x;
}

OutputStream& operator<< (OutputStream &s, state_model *e)
{
  if (e) s << e->Name();
  else s << "null";
  return s;
}

state_model::state_model(const char* n, int e)
{
  ALLOC("state_model", sizeof(state_model));
  name = n;
  statespace = NULL;
  events = e;
  proctype = Proc_Unknown;
  mc = NULL;
}

state_model::~state_model()
{
  FREE("state_model", sizeof(state_model));
  // Do NOT delete name, we are sharing it
  delete statespace;
  delete mc;
}

void state_model::DetermineProcessType()
{
  if (proctype != Proc_Unknown) return;
  // some stuff here

  proctype = Proc_General;
}

//@}


