
#ifndef OPS_SET_H
#define OPS_SET_H

/** \file ops_set.h

  Operator classes for sets, i.e., small sets of integers and such.
  These are sets used in for loops and as parameters in models:
    { 1 .. 5 }
    { 0.1 .. 1.0 .. 0.1 }
    { t1, t2, t3 }

  CTL state sets are defined elsewhere.
*/

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

class exprman;
void InitSetOps(exprman* em);


#endif

