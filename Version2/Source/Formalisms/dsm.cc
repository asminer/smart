
// $Id$

#include "dsm.h"

#include "../States/reachset.h"

/** @name dsm.cc
    @type File
    @args \ 

  Implementation for Discrete-state model "base class".

*/

//@{

OutputStream& operator<< (OutputStream &s, state_model *e)
{
  if (e) s << e->Name();
  else s << "null";
  return s;
}

state_model::state_model(const char* n, int e)
{
  name = n;
  statespace = NULL;
  events = e;
}

state_model::~state_model()
{
  // Do NOT delete name, we are sharing it
  delete statespace;
}

//@}


