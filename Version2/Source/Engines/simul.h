
// $Id$

#ifndef SIMUL_H
#define SIMUL_H

#include "../Language/models.h"

/** 	Solve a list of steady-state measures for a given model.

 	This is done by discrete-event simulation.

	@param  m	The model to solve.
	@param  mlist	A list of measures to compute.

	@return	true	if the simulation was successful
		false	if there was some error

*/
bool 	SimulateSteadyInst(model *m, List <measure> *mlist);

/// Initialize options for numerical solutions.
void InitSimulation();
#endif
