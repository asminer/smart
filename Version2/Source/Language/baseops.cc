
// $Id$

#include "baseops.h"
//@Include: baseops.h

/** @name baseops.cc
    @type File
    @args \ 

   Implementation of base expression classes for things like addition.

 */

//@{

// ******************************************************************
// *                                                                *
// *                          negop  class                          *
// *                                                                *
// ******************************************************************

void negop::show(OutputStream &s) const 
{
  type t = Type(0); // sneaky...  this is slow but so's I/O so who cares
  const char* op = "-";
  if ((t==BOOL) || (t==RAND_BOOL) || (t==PROC_BOOL) || (t==PROC_RAND_BOOL))
    op = "!";
  unary_show(s, op);
}

// ******************************************************************
// *                                                                *
// *                          addop  class                          *
// *                                                                *
// ******************************************************************

void addop::show(OutputStream &s) const 
{
  type t = Type(0); // sneaky...  this is slow but so's I/O so who cares
  const char* op = "+";
  if ((t==BOOL) || (t==RAND_BOOL) || (t==PROC_BOOL) || (t==PROC_RAND_BOOL))
    op = "|";
  assoc_show(s, op);
}

int addop::GetSums(int a, List <expr> *sums)
{
  DCASSERT(a==0);
  int i;
  int opnds=0;
  for (i=0; i<opnd_count; i++) {
    opnds += operands[i]->GetSums(a, sums);
  }
  return opnds;
}

// ******************************************************************
// *                                                                *
// *                          multop class                          *
// *                                                                *
// ******************************************************************

void multop::show(OutputStream &s) const 
{
  type t = Type(0); // sneaky...  this is slow but so's I/O so who cares
  const char* op = "*";
  if ((t==BOOL) || (t==RAND_BOOL) || (t==PROC_BOOL) || (t==PROC_RAND_BOOL))
    op = "&";
  assoc_show(s, op);
}

int multop::GetProducts(int a, List <expr> *prods)
{
  DCASSERT(a==0);
  int i;
  int opnds=0;
  for (i=0; i<opnd_count; i++) {
    opnds += operands[i]->GetProducts(a, prods);
  }
  return opnds;
}


//@}

