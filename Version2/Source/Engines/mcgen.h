
// $Id$

#ifndef MCGEN_H
#define MCGEN_H

#include "../Formalisms/dsm.h"

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
