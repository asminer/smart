
// $Id$

#include "strings.h"


// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

expr* MakeStringAdd(expr** opnds, int n, const char* file, int line);

expr* MakeStringBinary(expr* left, int op, expr* right, const char* file, int
line);

