
// $Id$

#include "dsm.h"

#include "../States/reachset.h"

/** @name dsm.cc
    @type File
    @args \ 

  Implementation for Discrete-state model "base class".

*/

//@{

state_model::state_model(int e)
{
  statespace = NULL;
  events = e;
}

state_model::~state_model()
{
  delete statespace;
}

//@}


