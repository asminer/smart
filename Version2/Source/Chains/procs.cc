
// $Id$

/*
   Stochastic process backend.
*/

#include "procs.h"




markov_chain::markov_chain()
{
  explicit_mc = NULL;
  initial = NULL;
}

markov_chain::~markov_chain()
{
  delete explicit_mc;
  delete initial;
}


