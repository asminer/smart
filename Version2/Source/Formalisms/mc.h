
// $Id$

/* Minimalist front-end. */

/** Build a Markov chain model.
*/

#include "api.h"

class PtrTable;

model* MakeMarkovChain(type t, char* id, formal_param **pl, int np,
			const char* fn, int line);

void InitMCModelFuncs(PtrTable *t);

