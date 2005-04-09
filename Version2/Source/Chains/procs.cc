
// $Id$

/*
   Stochastic process backend.
*/

#include "procs.h"

// *******************************************************************
// *                       reachgraph  methods                       *
// *******************************************************************

reachgraph::reachgraph()
{
  encoding = MC_None;
}

reachgraph::~reachgraph()
{
  switch (encoding) {
    case MC_Explicit:
 	delete explicit_rg;
	break;

    case MC_Kronecker:
	// delete kronecker
	break;

    default:
	// error or never initialized, do nothing
	// (keep compiler happy, though)
	break;
  }
}

void reachgraph::CreateExplicit(digraph *ex)
{
  DCASSERT(encoding == MC_None);
  encoding = MC_Explicit;
  explicit_rg = ex;
}

void reachgraph::CreateError()
{
  DCASSERT(encoding == MC_None);
  encoding = MC_Error;
}

// *******************************************************************
// *                      markov_chain  methods                      *
// *******************************************************************

markov_chain::markov_chain(sparse_vector <float> *init)
{
  encoding = MC_None;
  initial = init;
}

markov_chain::~markov_chain()
{
  switch (encoding) {
    case MC_Explicit:
 	delete explicit_mc;
	break;

    case MC_Kronecker:
	// delete kronecker
	break;

    default:
	// error or never initialized, do nothing
	// (keep compiler happy, though)
	break;
  }
  delete initial;
}

void markov_chain::CreateExplicit(classified_chain<float> *ex)
{
  DCASSERT(encoding == MC_None);
  encoding = MC_Explicit;
  explicit_mc = ex;
}

void markov_chain::CreateError()
{
  DCASSERT(encoding == MC_None);
  encoding = MC_Error;
}

option* MatrixByRows = NULL;

// *******************************************************************
// *                        global  functions                        *
// *******************************************************************

void InitProcOptions() 
{
  if (MatrixByRows) return; // in case we are called twice
  MatrixByRows = MakeBoolOption("MatrixByRows", "Should sparse matrices be stored by rows", false);
  AddOption(MatrixByRows);

  // MarkovStorage and such are defined in Engines/mcgen
}

