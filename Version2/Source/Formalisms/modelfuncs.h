
// $Id$

#ifndef MODELFUNCS_H
#define MODELFUNCS_H

#include "../Main/tables.h"

/**	Add internal (builtin) functions to symbol table t.
	This is for functions common to all models,
	such as "avg_ss".
*/
void InitGenericModelFunctions(PtrTable *t);

#endif
