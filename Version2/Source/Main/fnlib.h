
// $Id$

#ifndef FNLIB_H
#define FNLIB_H

#include "tables.h"

/**	Add internal (builtin) functions to symbol table t.
	This is all functions except those that deal with models.
*/
void InitBuiltinFunctions(PtrTable *t);

/**	Add builtin constants to symbol table t.
        Example: infty.
*/
void InitBuiltinConstants(PtrTable *t);

#endif
