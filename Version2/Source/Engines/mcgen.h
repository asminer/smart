
// $Id$

#ifndef MCGEN_H
#define MCGEN_H

#include "../Formalisms/dsm.h"
#include "../Templates/graphs.h"

// Necessary for ssmcgen.cc

extern option* MarkovStorage;
extern option_const sparse_mc;
extern option_const kronecker_mc;

/** Useful helper function.
    Compress and attach the CTMC the state model.
    If an error occurred, use mc = NULL.
*/
void CompressAndAffix(state_model* dsm, labeled_digraph<float> *mc);

/** 	Build the reachability set for a state model.

        This will be added to the state model itself.
	If there was a problem, a special "error" reachability
	set will be set for the model.
	(This allows errors to propogate in a healty way.)

*/
void 	BuildCTMC(state_model *dsm);

/// Initialize options for reachability graph / Markov chain generation.
void InitMCGen();

#endif
