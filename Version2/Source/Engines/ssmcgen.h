
// $Id$

#ifndef SSMCGEN_H
#define SSMCGEN_H

#include "../Formalisms/dsm.h"

/** 	Build the reachability set AND reachability graph / Markov chain
        for a state model.

        Simultaneous construction, for certain "compatible" pairs
        of state storage and Markov storage options.
        
        If simultaneous construction is impossible, we will
        call the state space generation algorithm first
        (implemented in ssgen)
        and then the reachability graph / Markov chain generation algorithm
        (implemented in mcgen).

	The state space and graph/chain will be added to the model itself.

*/
void 	BuildReachsetAndCTMC(state_model *dsm);

/// Same as BuildReachsetAndCTMC, but for reachability graph.
void 	BuildReachSetAndGraph(state_model *dsm);

#endif
