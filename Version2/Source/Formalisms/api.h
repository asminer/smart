
// $Id$

#ifndef FORMALISMS_API
#define FORMALISMS_API

#include "../Language/models.h"

/*
	Frontends for formalisms.

	Basically, whenever you define a new formalism,
	you will need to update these functions.
 */

/** 	Create a model (language-wise) of the specific type.

	@param	fn	Filename of declaration
	@param	line	Line number of declaration
	@param	t	Type of model
	@param	name	Name of model
	@param	pl	Array of formal parameters (or NULL)
	@param	np	Number of formal parameters
*/
model*	MakeNewModel(const char* fn, int line, 
			type t, char* name, formal_param **pl, int np);

#endif
