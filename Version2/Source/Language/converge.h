
// $Id$

/** @name converge.h
    @type File
    @args \ 

  Everything to do with converge statements.
 
*/


#ifndef CONVERGE_H
#define CONVERGE_H

// front-ends here

class cvgfunc;

// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

cvgfunc* MakeConvergeVar(type t, char* id, const char* file, int line);

#endif
