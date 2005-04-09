
// $Id$

/*
   Stochastic process (minimalist) frontend.
   Same philosophy as reachability sets.
*/

#ifndef PROCS_H
#define PROCS_H

#include "mc_expl.h"
#include "../Base/options.h"
#include "../Templates/sparsevect.h"

/// Storage options for a Markov chain or Reachability graph
enum mc_storage {
  /// Empty (use for initializing)
  MC_None,
  /// Explicit (sparse) storage.
  MC_Explicit,
  /// Kronecker (eventually)
  MC_Kronecker,
  // others here

  /// There was an error generating it.
  MC_Error
};

class reachgraph {
  mc_storage encoding;
  union {
    /// Used by explicit storage
    digraph *explicit_rg;

    /// Implement later...
    void* kron;
  };
// initial states?
public:
  reachgraph();
  ~reachgraph();

  inline mc_storage Storage() const { return encoding; }

  /// Create an explicit rg
  void CreateExplicit(digraph* ex);
  /// Create an error rg
  void CreateError();

  inline digraph* Explicit() const {
    DCASSERT(encoding == MC_Explicit);
    return explicit_rg;
  }

};

class markov_chain {
  /// how it is stored.
  mc_storage encoding;
  union {
    /// Used by explicit storage
    classified_chain <float>  *explicit_mc;

    /// Implement later...
    void* kron;
  };
public:
  sparse_vector <float> *initial; 
public:
  markov_chain(sparse_vector <float> *init);
  ~markov_chain();

  inline mc_storage Storage() const { return encoding; }

  /// Create an explicit mc
  void CreateExplicit(classified_chain<float> *ex);
  /// Create an error mc
  void CreateError();

  inline classified_chain<float>* Explicit() const {
    DCASSERT(encoding == MC_Explicit);
    return explicit_mc;
  }
};

extern option* MatrixByRows;

void InitProcOptions();

#endif
