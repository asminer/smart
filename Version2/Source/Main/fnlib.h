
// $Id$

#ifndef FNLIB_H
#define FNLIB_H

#include "tables.h"

/**	Add internal (builtin) functions to symbol table t.
	This is all functions except those that deal with models.
*/
void InitBuiltinFunctions(PtrTable *t);

#endif
