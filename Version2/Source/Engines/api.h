
// $Id$

#ifndef ENGINES_API
#define ENGINES_API

#include "../Language/models.h"

/*
	Engine frontend functions.

 */

/** 	Solve a list of steady-state measures for a given model.

        (Do we need more parameters or a return value?)

	@param  m	The model to solve.
	@param  mlist	A list of measures to compute.

	Solution method is determined by option (TBD).

	Measure solutions are stored within the measures 
        themselves (in the list).

	If there is any kind of error / problem with the solution,
        all measures should have their return value results set to
	the error state.
*/
void 	SolveSteady(model *m, List <measure> *mlist);

#endif
