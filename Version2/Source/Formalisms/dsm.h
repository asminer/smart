
// $Id$

#ifndef DSM_H
#define DSM_H

/** @name dsm.h
    @type File
    @args \ 

  Discrete-state model.
  The meta model class used by solution engines.

  Hopefully this can avoid virtual functions for the critical operations.

  We will still use virtual functions and derived classes for:
	Showing states

*/

//@{

class reachset;  // defined in States/reachset.h

class state_model {
  /// Reachable states.
  reachset *statespace; 
  // Reachability graph?
  // Markov chain here?
public:
  state_model();
  ~state_model();
};

//@}

#endif

