
// $Id$

/* Minimalist front-end for spns. */

/** Build a Stochastic Petri net model.
*/

#include "api.h"

class PtrTable;

model* MakePetriNet(type t, char* id, formal_param **pl, int np,
			const char* fn, int line);

void InitPNModelFuncs(PtrTable *t);

