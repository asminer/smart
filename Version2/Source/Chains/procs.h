
// $Id$

/*
   Stochastic process (minimalist) frontend.
*/

#ifndef PROCS_H
#define PROCS_H

#include "mc_expl.h"
#include "../Base/options.h"
#include "../Templates/sparsevect.h"

enum Process_type {
  /// Placeholder; we don't know yet
  Proc_Unknown,
  /// An unsupported process, or an error occurred
  Proc_Error,
  /// Finite state machine (no timing, for model checking)
  Proc_FSM,
  /// Discrete-time Markov chain
  Proc_Dtmc,
  /// Continuous-time Markov chain
  Proc_Ctmc,
  // other types, for Rob?

  /// General stochastic process (use simulation)
  Proc_General
};


struct markov_chain {

  // Not sure how to organize this yet...
  classified_chain <float>  *explicit_mc;

  // implicit types (kronecker, etc.) here

  sparse_vector <float> *initial; 

  markov_chain();
  ~markov_chain();

};

extern option* MatrixByRows;

void InitProcOptions();

#endif
