
// $Id$

#ifndef ENGINES_API
#define ENGINES_API

#include "../Language/models.h"

/*
	Engine frontend functions.

 */

/** 	Build the reachability set for a given model.
	The reachability set is added to the state model.
	@param  m	The model to solve.
*/
void 	BuildReachset(model *m);

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
void 	SolveSteadyInst(model *m, List <measure> *mlist);

/** 	Solve a list of steady-state accumulated measures for a given model.
	@param  m	The model to solve.
	@param  mlist	A list of measures to compute.
*/
void	SolveSteadyAcc(model *m, List <measure> *mlist);

/** 	Solve a list of transient measures for a given model.
	@param  m	The model to solve.
	@param  mlist	A list of measures to compute.
*/
void	SolveTransientInst(model *m, List <measure> *mlist);

/** 	Solve a list of transient accumulated measures for a given model.
	@param  m	The model to solve.
	@param  mlist	A list of measures to compute.
*/
void	SolveTransientAcc(model *m, List <measure> *mlist);


/** 	Used to initialize options for solution engines.
*/
void InitEngines();

#endif
