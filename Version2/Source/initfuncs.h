
// $Id$

#ifndef INITFUNCS_H
#define INITFUNCS_H

#include "functions.h"

/** @name initfuncs.h
    @type File
    @args \ 

  Initialization of built-in functions 
  (except those related to models)
  is here.

 */

//@{
  
/** Initialize the built-in functions.
    We need a fancy type of symbol table;
    for now, it is just an array of functions.
 */
void InitFuncs(internal_func **table, int tabsize);

//@}

#endif

