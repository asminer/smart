
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


option* MatrixByRows = NULL;

void InitProcOptions() 
{
  if (MatrixByRows) return; // in case we are called twice
  MatrixByRows = MakeBoolOption("MatrixByRows", "Should sparse matrices be stored by rows", false);
  AddOption(MatrixByRows);

  // MarkovStorage and such?
}

