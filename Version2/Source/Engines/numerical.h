
// $Id$

#ifndef NUMERICAL_H
#define NUMERICAL_H

#include "../Language/models.h"

/** 	Solve a list of steady-state measures for a given model.

 	This is done by numerically analyzing 
	the underlying stochastic process.

	@param  m	The model to solve.
	@param  mlist	A list of measures to compute.

	@return	true	if solution was successful
		false	if there was some error

*/
bool 	NumericalSteadyInst(model *m, List <measure> *mlist);

/// Initialize options for numerical solutions.
void InitNumerical();

#endif
