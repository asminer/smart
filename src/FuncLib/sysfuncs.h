
// $Id$

/** \file sysfuncs.h

  System related functions.

*/

#ifndef SYSFUNCS_H
#define SYSFUNCS_H

class symbol_table;
class exprman;

void AddSysFunctions(symbol_table* st, const exprman* em, const char** env, const char* version);

#endif

