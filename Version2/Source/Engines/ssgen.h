
// $Id$

#ifndef SSGEN_H
#define SSGEN_H

#include "../Formalisms/dsm.h"

/** 	Build the reachability set for a state model.

        If successful, this will be added to the state model itself.

	@return true on success.
*/
bool 	BuildReachset(state_model *dsm);

/// Initialize options for reachability set generation.
void InitSSGen();

#endif
