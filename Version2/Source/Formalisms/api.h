
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

/** 	Checks if a desired model variable is allowed
	within a specific model type.

	@param	modeltype	The type of the model
	@param	vartype		The type of the variable

	@return	true	If the variable is allowed within the model
*/

bool	CanDeclareType(type modeltype, type vartype);

/**	
	Find the list of built-in functions with name n, for
	a model of given type.
	Used by the compiler.
*/
List <function> *FindModelFunctions(type modeltype, const char* n);

/**	Initializes model formalism stuff.
*/
void InitModels();

#endif
